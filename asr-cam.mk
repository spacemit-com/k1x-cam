ASR_CAM_VERSION = 1.0.0
ASR_CAM_SITE = $(TOPDIR)/package/asr-cam
ASR_CAM_SITE_METHOD = local
ASR_CAM_INSTALL_TARGET = YES

define ASR_CAM_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 package/asr-cam/libs/lib64/libcppfw-2.0.so $(TARGET_DIR)/usr/lib/libcppfw-2.0.so
	$(INSTALL) -D -m 0755 package/asr-cam/libs/lib64/libcpp.so $(TARGET_DIR)/usr/lib/libcpp.so
	$(INSTALL) -D -m 0755 package/asr-cam/libs/lib64/libispeagle.so $(TARGET_DIR)/usr/lib/libispeagle.so
	$(INSTALL) -D -m 0755 package/asr-cam/libs/lib64/libisp.so $(TARGET_DIR)/usr/lib/libisp.so
	$(INSTALL) -D -m 0755 package/asr-cam/libs/lib64/libtuningtools.so $(TARGET_DIR)/usr/lib/libtuningtools.so
	$(INSTALL) -D -m 0755 package/asr-cam/libs/lib64/libvi.so $(TARGET_DIR)/usr/lib/libvi.so

	$(INSTALL) -D -m 0755 $(@D)/sensors/libcam_sensors.so $(TARGET_DIR)/usr/lib/libcam_sensors.so
	$(INSTALL) -D -m 0755 $(@D)/sensors/test/cam_sensors_test $(TARGET_DIR)/usr/bin/cam_sensors_test
	$(INSTALL) -D -m 0755 $(@D)/demo/sdkTest $(TARGET_DIR)/usr/bin/sdkTest
	$(INSTALL) -D -m 0755 $(@D)/demo/sdktest.sh $(TARGET_DIR)/usr/bin/sdktest.sh
endef

$(eval $(cmake-package))

