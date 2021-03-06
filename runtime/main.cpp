#include <vector>
#include <GL/glew.h>
#include <GL/glx.h>
#include <memory>
#include <random>
#include <chrono>
#include <future>
#include <thread>
#include "common/plugin.hpp"
#include "common/data_format.hpp"
#include "common/extended_window.hpp"
#include "dynamic_lib.hpp"
#include "phonebook_impl.hpp"
#include "switchboard_impl.hpp"

using namespace ILLIXR;

static constexpr int   EYE_TEXTURE_WIDTH   = 1024;
static constexpr int   EYE_TEXTURE_HEIGHT  = 1024;


// Temporary OpenGL-specific code for creating shared OpenGL context.
// May be superceded in the future by more modular, Vulkan-based resource management.

std::unique_ptr<phonebook> pb;
// I have to keep the dynamic libs in scope until the program is dead
std::vector<dynamic_lib> libs;
std::vector<std::unique_ptr<plugin>> plugins;

extern "C" int illixrrt_init(void* appGLCtx) {
	/* TODO: use a config-file instead of cmd-line args. Config file
	   can be more complex and can be distributed more easily (checked
	   into git repository). */

	pb = create_phonebook();
	auto sb = create_switchboard().release();
	
	pb->register_impl<switchboard>(sb);
	pb->register_impl<xlib_gl_extended_window>(new xlib_gl_extended_window {448*2, 320*2, (GLXContext)appGLCtx});
	// pb->register_impl<global_config>(new global_config {headless_window});
	return 0;
}

extern "C" void illixrrt_load_plugin(const char *path) {
	auto lib = dynamic_lib::create(std::string_view{path});
	plugin* p = lib.get<plugin* (*) (phonebook*)>("plugin_main")(pb.get());
	plugins.emplace_back(p);
	libs.push_back(std::move(lib));
}

extern "C" void illixrrt_attach_plugin(plugin* (*f) (phonebook*)) {
	plugin* p = f(pb.get());
	plugins.emplace_back(p);
}

int main(int argc, char **argv) {
	if (illixrrt_init(NULL) != 0) {
		return 1;
	}
	for (int i = 1; i < argc; ++i) {
		illixrrt_load_plugin(argv[i]);
	}
	while (true) { }
	return 0;
}
