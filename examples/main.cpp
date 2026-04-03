#include <glm/gtc/matrix_transform.hpp>

#include "MPVGL/Core/RenderWindow.hpp"
#include "MPVGL/Core/Scene.hpp"
#include "MPVGL/Core/WindowConfig.hpp"
#include "MPVGL/Geometry/Shape.hpp"

int main() {
    auto config = mpvgl::WindowConfig{};
    mpvgl::Scene scene{};

    scene.add(mpvgl::Shape<mpvgl::Cube>::generate(mpvgl::Color::Literal::Red),
              glm::translate(glm::mat4(1.0F), glm::vec3(-2.0F, 0.0F, 0.0F)));

    scene.add(mpvgl::Shape<mpvgl::Cube>::generate(mpvgl::Color::Literal::Green),
              glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 0.0F, 0.0F)));

    scene.add(mpvgl::Shape<mpvgl::Quad>::generate(mpvgl::Color::Literal::Blue),
              glm::translate(glm::mat4(1.0F), glm::vec3(2.0F, 1.0F, 0.0F)));

    mpvgl::RenderWindow window(config, scene);
    return window.draw();
}
