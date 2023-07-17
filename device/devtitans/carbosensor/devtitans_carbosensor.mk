# Herda as configurações do emulador (produto sdk_phone_x86_64)
$(call inherit-product, $(SRC_TARGET_DIR)/product/sdk_phone_x86_64.mk)

# Sobrescreve algumas variáveis com os dados do novo produto
PRODUCT_NAME := devtitans_carbosensor
PRODUCT_DEVICE := carbosensor
PRODUCT_BRAND := carboSensorBrand
PRODUCT_MODEL := carboSensorModel

# Copia o arquivo devtitans.txt para o /system/etc da imagem do Android
#PRODUCT_COPY_FILES += \
##	device/devtitans/kraken/devtitans.txt:system/etc/devtitans.txt \
#	device/devtitans/kraken/kraken.rc:vendor/etc/init/kraken.rc \
#	device/devtitans/kraken/bootanimation.zip:product/media/bootanimation.zip \
#	device/devtitans/kraken/default_wallpaper2.png:product/media/wallpaper/default_wallpaper2.png

PRODUCT_SYSTEM_PROPERTIES += \
	ro.devtitans.name=carboSensor

PRODUCT_PRODUCT_PROPERTIES += \
	ro.product.devtitans.version=1.0

#PRODUCT_VENDOR_PROPERTIES += \
#	ro.vendor.devtitans.hardware=ModelB

#PRODUCT_PRODUCT_PROPERTIES += \
#	ro.config.wallpaper=product/media/wallpaper/default_wallpaper2.png

# Seta o diretório de overlays
PRODUCT_PACKAGE_OVERLAYS = device/devtitans/carbosensor/overlay

PRODUCT_PACKAGES += \
	UniversalMediaPlayer \

#BOARD_SEPOLICY_DIRS += device/devtitans/carbosensor/sepolicy

# Smartlamp AIDL Interface
#PRODUCT_PACKAGES += devtitans.smartlamp

# Smartlamp Binder Service
#PRODUCT_PACKAGES += devtitans.smartlamp-service

# Device Framework Matrix (Declara que o nosso produto Kraken precisa do serviço smartlamp)
#DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE := device/devtitans/carbosensor/device_framework_matrix.xml
