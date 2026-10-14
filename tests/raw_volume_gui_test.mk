# Run in v3d_main/v3d after the normal build.
.PHONY: raw-volume-gui-test
raw-volume-gui-test: /tmp/vaa3d-raw-volume-gui
/tmp/vaa3d-raw-volume-gui.o: ../../tests/raw_volume_gui_test.cpp ChannelTable.h xformwidget.h v3d_core.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<
/tmp/vaa3d-raw-volume-gui: /tmp/vaa3d-raw-volume-gui.o $(filter-out main.o,$(OBJECTS))
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)
