#ifndef CS_APP_HPP
#define CS_APP_HPP

struct size { int width, height; };

void init() noexcept;
void deinit() noexcept;
[[noreturn]] void terminate(char const * message) noexcept;
[[nodiscard]] bool new_frame() noexcept; // whether to keep running
void render() noexcept;
[[nodiscard]] size framebuffer_size() noexcept;

#endif // CS_APP_HPP
