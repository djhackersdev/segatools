$(BUILD_DIR_ZIP)/chuni.zip:
	$(V)echo ... $@
	$(V)mkdir -p $(BUILD_DIR_ZIP)/chuni
	$(V)mkdir -p $(BUILD_DIR_ZIP)/chuni/DEVICE
	$(V)cp $(BUILD_DIR_32)/subprojects/capnhook/inject/inject.exe \
		$(BUILD_DIR_32)/chunihook/chunihook.dll \
		$(DIST_DIR)/chuni/segatools.ini \
		$(DIST_DIR)/chuni/start.bat \
		$(BUILD_DIR_ZIP)/chuni
	$(V)cp pki/billing.pub \
		pki/ca.crt \
    	$(BUILD_DIR_ZIP)/chuni/DEVICE
	$(V)strip $(BUILD_DIR_ZIP)/chuni/*.{exe,dll}
	$(V)cd $(BUILD_DIR_ZIP)/chuni ; zip -r ../chuni.zip *

$(BUILD_DIR_ZIP)/idz.zip:
	$(V)echo ... $@
	$(V)mkdir -p $(BUILD_DIR_ZIP)/idz
	$(V)mkdir -p $(BUILD_DIR_ZIP)/idz/DEVICE
	$(V)cp $(BUILD_DIR_64)/subprojects/capnhook/inject/inject.exe \
		$(BUILD_DIR_64)/idzhook/idzhook.dll \
		$(DIST_DIR)/idz/segatools.ini \
		$(DIST_DIR)/idz/start.bat \
    	$(BUILD_DIR_ZIP)/idz
	$(V)cp pki/billing.pub \
		pki/ca.crt \
    	$(BUILD_DIR_ZIP)/idz/DEVICE
	$(V)strip $(BUILD_DIR_ZIP)/idz/*.{exe,dll}
	$(V)cd $(BUILD_DIR_ZIP)/idz ; zip -r ../idz.zip *

$(BUILD_DIR_ZIP)/doc.zip: \
		$(DOC_DIR)/config \
		$(DOC_DIR)/chunihook.md \
		$(DOC_DIR)/idzhook.md \
		| $(zipdir)/
	$(V)echo ... $@
	$(V)zip -r $@ $^

$(BUILD_DIR_ZIP)/segatools.zip: \
		$(BUILD_DIR_ZIP)/chuni.zip \
		$(BUILD_DIR_ZIP)/doc.zip \
		$(BUILD_DIR_ZIP)/idz.zip \
		CHANGELOG.md \
		README.md \

	$(V)echo ... $@
	$(V)zip -j $@ $^
