#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>

MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver de acesso ao CarboSensor (ESP32 com Chip Serial CP2102");
MODULE_LICENSE("GPL");

// Tamanho máximo de uma linha de resposta do dispositvo USB
#define MAX_RECV_LINE 100

static int  usb_probe(struct usb_interface *ifce, const struct usb_device_id *id); // Executado quando o dispositivo é conectado na USB
static void usb_disconnect(struct usb_interface *ifce);                            // Executado quando o dispositivo USB é desconectado da USB
static int  usb_send_cmd(char *cmd, int param);                                    // Envia um comando via USB e espera/retorna a resposta do dispositivo (int)

// Executado quando o arquivo /sys/kernel/carbosensor/{mq7} é lido (e.g., cat /sys/kernel/carbosensor/mq7)
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff);

// Executado quando o arquivo /sys/kernel/carbosensor/{mq7} é escrito (e.g., echo "100" | sudo tee -a /sys/kernel/carbosensor/mq7)
//static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count);

static char recv_line[MAX_RECV_LINE];              // Armazena dados vindos da USB até receber um caractere de nova linha '\n'
static struct usb_device *carbosensor_device;        // Referência para o dispositivo USB
static uint usb_in, usb_out;                       // Endereços das portas de entrada e saida da USB
static char *usb_in_buffer, *usb_out_buffer;       // Buffers de entrada e saída da USB
static int usb_max_size;                           // Tamanho máximo de uma mensagem USB

// Variáveis para criar os arquivos no /sys/kernel/carbosensor/{mq7}
//static struct kobj_attribute  mq7_attribute = __ATTR(mq7, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct kobj_attribute  mq7_attribute = __ATTR(mq7, S_IRUGO | S_IWUSR, attr_show,NULL);
static struct attribute      *attrs[]       = { &mq7_attribute.attr, NULL };
static struct attribute_group attr_group    = { .attrs = attrs };
static struct kobject        *sys_obj;

// Registra o CP2102 (Chip USB-Serial do ESP32) no USB-Core do kernel
#define VENDOR_ID  0x10C4  /* VendorID  do CP2102 */
#define PRODUCT_ID 0xEA60  /* ProductID do CP2102 */
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };
MODULE_DEVICE_TABLE(usb, id_table);

// Cria e registra o driver do carbosensor no kernel
static struct usb_driver carbosensor_driver = {
    .name        = "carbosensor",     // Nome do driver
    .probe       = usb_probe,       // Executado quando o dispositivo é conectado na USB
    .disconnect  = usb_disconnect,  // Executado quando o dispositivo é desconectado na USB
    .id_table    = id_table,        // Tabela com o VendorID e ProductID do dispositivo
};
module_usb_driver(carbosensor_driver);

// Executado quando o dispositivo é conectado na USB
static int usb_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    int ret;
    int retval;
    
    struct usb_endpoint_descriptor *usb_endpoint_in, *usb_endpoint_out;

    printk(KERN_INFO "CarboSensor: Dispositivo conectado ...\n");

    // Cria arquivos do /sys/kernel/carbosensor/*
    sys_obj = kobject_create_and_add("carbosensor", kernel_kobj);
    ret = sysfs_create_group(sys_obj, &attr_group);

    // Detecta portas e aloca buffers de entrada e saída de dados na USB
    carbosensor_device = interface_to_usbdev(interface);
    retval = usb_find_common_endpoints(interface->cur_altsetting, &usb_endpoint_in, &usb_endpoint_out, NULL, NULL);
    usb_max_size = usb_endpoint_maxp(usb_endpoint_in);
    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;
    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);

    return 0;
}

// Executado quando o dispositivo USB é desconectado da USB
static void usb_disconnect(struct usb_interface *interface) {
    printk(KERN_INFO "SmartLamp: Dispositivo desconectado.\n");
    if (sys_obj) kobject_put(sys_obj);      // Remove os arquivos em /sys/kernel/carbosensor
    kfree(usb_in_buffer);                   // Desaloca buffers
    kfree(usb_out_buffer);
}

// Envia um comando via USB, espera e retorna a resposta do dispositivo (convertido para int)
// Exemplo de Comando:  GET_MQ7
static int usb_send_cmd(char *cmd, int param) {
    int recv_size = 0;                      // Quantidade de caracteres no recv_line
    int ret, actual_size, i;
    int retries = 10;                       // Tenta algumas vezes receber uma resposta da USB. Depois desiste.
    char resp_expected[MAX_RECV_LINE];      // Resposta esperada do comando
    char *resp_pos;                         // Posição na linha lida que contém o número retornado pelo dispositivo
    long resp_number = -1;                  // Número retornado pelo dispositivo (e.g., valor do mq7)
    long retkstrtol;

    printk(KERN_INFO "CarboSensor: Enviando comando: %s\n", cmd);

    if (param >= 0) sprintf(usb_out_buffer, "%s %d\n", cmd, param); // Se param >=0, o comando possui um parâmetro (int)
    else sprintf(usb_out_buffer, "%s\n", cmd);                      // Caso contrário, é só o comando mesmo

    // Envia o comando (usb_out_buffer) para a USB
    ret = usb_bulk_msg(carbosensor_device, usb_sndbulkpipe(carbosensor_device, usb_out), usb_out_buffer, strlen(usb_out_buffer), &actual_size, 1000*HZ);
    if (ret) {
        printk(KERN_ERR "CarboSensor: Erro de codigo %d ao enviar comando!\n", ret);
        return -1;
    }

    sprintf(resp_expected, "RES %s", cmd);  // Resposta esperada. Ficará lendo linhas até receber essa resposta.

    // Espera pela resposta correta do dispositivo (desiste depois de várias tentativas)
    while (retries > 0) {
        // Lê dados da USB
        
        ret = usb_bulk_msg(carbosensor_device, usb_rcvbulkpipe(carbosensor_device, usb_in), usb_in_buffer, min(usb_max_size, MAX_RECV_LINE), &actual_size, HZ*1000);
        if (ret) {
            printk(KERN_ERR "CarboSensor: Erro ao ler dados da USB (tentativa %d). Codigo: %d\n", ret, retries--);
            continue;
        }

        // Para cada caractere recebido ...
        for (i=0; i<actual_size; i++) {

            if (usb_in_buffer[i] == '\n') {  // Temos uma linha completa
                recv_line[recv_size] = '\0';
                printk(KERN_INFO "CarboSensor: Recebido uma linha: '%s'\n", recv_line);

                // Verifica se o início da linha recebida é igual à resposta esperada do comando enviado
                if (!strncmp(recv_line, resp_expected, strlen(resp_expected))) {
                    printk(KERN_INFO "CarboSensor: Linha eh resposta para %s! Processando ...\n", cmd);

                    // Acessa a parte da resposta que contém o número e converte para inteiro
                    resp_pos = &recv_line[strlen(resp_expected) + 1];
                    retkstrtol = kstrtol(resp_pos, 10, &resp_number);

                    return resp_number;
                }
                else { // Não é a linha que estávamos esperando. Pega a próxima.
                    printk(KERN_INFO "CarboSensor: Nao eh resposta para %s! Tentiva %d. Proxima linha...\n", cmd, retries--);
                    recv_size = 0; // Limpa a linha lida (recv_line)
                }
            }
            else { // É um caractere normal (sem ser nova linha), coloca no recv_line e lê o próximo caractere
                recv_line[recv_size] = usb_in_buffer[i];
                recv_size++;
            }
        }
    }
    return -1; // Não recebi a resposta esperada do dispositivo
}

// Executado quando o arquivo /sys/kernel/carbosensor/{mq7} é lido (e.g., cat /sys/kernel/carbosensor/mq7)
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff) {
    int value;
    const char *attr_name = attr->attr.name;

    printk(KERN_INFO "CarboSensor: Lendo %s ...\n", attr_name);

    value = usb_send_cmd("GET_MQ7", -1);

    sprintf(buff, "%d\n", value);                   // Cria a mensagem com o valor do mq7
    return strlen(buff);
}

// Executado quando o arquivo /sys/kernel/carbosensor/{mq7} é escrito (e.g., echo "100" | sudo tee -a /sys/kernel/carbosensor/mq7)
// static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count) {
//     long ret, value;
//     const char *attr_name = attr->attr.name;
//
//     ret = kstrtol(buff, 10, &value);
//     if (ret) {
//         printk(KERN_ALERT "CarboSensor: valor de %s invalido.\n", attr_name);
//         return -EACCES;
//     }
//
//     printk(KERN_INFO "CarboSensor: Setando %s para %ld ...\n", attr_name, value);
//
//     if (!strcmp(attr_name, "led"))
//         ret = usb_send_cmd("SET_LED", value);
//     else {
//         printk(KERN_ALERT "CarboSensor: o valor do mq7 (sensor de monoxido) eh apenas para leitura.\n");
//         return -EACCES;
//     }
//     if (ret < 0) {
//         printk(KERN_ALERT "CarboSensor: erro ao setar o valor do %s.\n", attr_name);
//         return -EACCES;
//     }
//
//     return strlen(buff);
// }
