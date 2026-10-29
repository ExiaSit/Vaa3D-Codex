# Run from v3d_main/v3d after a normal build.
.PHONY: cut-move-gui-test
cut-move-gui-test: /tmp/vaa3d-cut-move-gui
/tmp/vaa3d-cut-move-gui.o: ../../tests/cut_move_gui_test.cpp ../3drenderer/v3dr_glwidget.h ../3drenderer/v3dr_mainwindow.h ../basic_c_fun/basic_surf_objs.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<
/tmp/vaa3d-cut-move-gui: /tmp/vaa3d-cut-move-gui.o $(filter-out main.o,$(OBJECTS))
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)
