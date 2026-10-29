# Run in v3d_main/v3d after a normal build.
.PHONY: mask-async-gui-test
mask-async-gui-test: /tmp/vaa3d-mask-async-gui
/tmp/vaa3d-mask-async-gui.o: ../../tests/mask_async_gui_test.cpp ../3drenderer/renderer_gl1.h ../basic_c_fun/basic_surf_objs.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<
/tmp/vaa3d-mask-async-gui: /tmp/vaa3d-mask-async-gui.o $(filter-out main.o,$(OBJECTS))
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)
