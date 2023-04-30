#include "app.hpp"
#include "managers.hpp"

#if 0
namespace manager = slime;
#else
namespace manager = rt;
#endif

int main() {
	init();
	manager::init();
	while(new_frame()) {
		manager::compute();
		render();
	}
	manager::shutdown();
	shutdown();
}
