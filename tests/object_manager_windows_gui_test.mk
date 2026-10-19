# Run in v3d_main/v3d after a normal build.
.PHONY: object-manager-windows-gui-test
object-manager-windows-gui-test: /tmp/vaa3d-object-manager-windows-gui
/tmp/vaa3d-object-manager-windows-gui.o: ../../tests/object_manager_windows_gui_test.cpp ../3drenderer/v3dr_glwidget.h ../3drenderer/v3dr_surfaceDialog.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<
/tmp/vaa3d-object-manager-windows-gui: /tmp/vaa3d-object-manager-windows-gui.o $(filter-out main.o,$(OBJECTS))
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)
