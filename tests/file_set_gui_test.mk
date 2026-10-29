.PHONY: file-set-gui-test
file-set-gui-test: /tmp/vaa3d-file-set-gui
/tmp/vaa3d-file-set-gui.o: ../../tests/file_set_gui_test.cpp OpenFileSet.h main.cpp ../3drenderer/renderer_gl1.h ../3drenderer/v3dr_glwidget.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<
/tmp/vaa3d-file-set-gui: /tmp/vaa3d-file-set-gui.o $(filter-out main.o,$(OBJECTS))
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)
