.PHONY: local-volume-hide-gui-test
local-volume-hide-gui-test: /tmp/vaa3d-local-volume-hide-gui
/tmp/vaa3d-local-volume-hide-gui.o: ../../tests/local_volume_hide_gui_test.cpp ../3drenderer/LocalVolumeHide.h ../3drenderer/v3dr_glwidget.h ../3drenderer/renderer_gl1.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<
/tmp/vaa3d-local-volume-hide-gui: /tmp/vaa3d-local-volume-hide-gui.o $(filter-out main.o,$(OBJECTS))
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)
