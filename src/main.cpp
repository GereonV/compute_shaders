#include "app.hpp"
#include "managers.hpp"

int main() {
	init();
	slime::init();
	while(new_frame()) {
		slime::compute();
		render();
	}
	slime::shutdown();
	shutdown();
}
