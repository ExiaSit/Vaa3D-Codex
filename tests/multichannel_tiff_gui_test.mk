# Run in v3d_main/v3d after the normal build.
.PHONY: multichannel-tiff-gui-test
multichannel-tiff-gui-test: /tmp/vaa3d-multichannel-tiff-gui
/tmp/vaa3d-multichannel-tiff-gui.o: ../../tests/multichannel_tiff_gui_test.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<
/tmp/vaa3d-multichannel-tiff-gui: /tmp/vaa3d-multichannel-tiff-gui.o $(filter-out main.o,$(OBJECTS))
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)
