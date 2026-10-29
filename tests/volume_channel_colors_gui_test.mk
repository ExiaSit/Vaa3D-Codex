.PHONY: volume-channel-colors-gui-test
volume-channel-colors-gui-test: /tmp/vaa3d-volume-channel-colors-gui
/tmp/vaa3d-volume-channel-colors-gui.o: ../../tests/volume_channel_colors_gui_test.cpp ../3drenderer/v3dr_colormapDialog.h ../3drenderer/renderer_gl2.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<
/tmp/vaa3d-volume-channel-colors-gui: /tmp/vaa3d-volume-channel-colors-gui.o $(filter-out main.o,$(OBJECTS))
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)
