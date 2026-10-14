# Run from v3d_main/v3d after a normal build:
# make -f Makefile -f ../../tests/triview_resize_gui_test.mk triview-resize-gui-test
# (Correct relative path from v3d_main/v3d is ../../tests.)
.PHONY: triview-resize-gui-test
triview-resize-gui-test: /tmp/vaa3d-triview-resize-gui
/tmp/vaa3d-triview-resize-gui.o: ../../tests/triview_resize_gui_test.cpp xformwidget.h v3d_core.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<
/tmp/vaa3d-triview-resize-gui: /tmp/vaa3d-triview-resize-gui.o $(filter-out main.o,$(OBJECTS))
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)
