#pragma once

namespace mpvgl {

template <>
inline Mesh<Vertex> Shape<Quad>::generate(Color const& color) {
    Mesh<Vertex> mesh;
    mesh.vertices.reserve(ShapeTraits<Quad>::vertexCount);
    mesh.indices.reserve(ShapeTraits<Quad>::indexCount);

    mesh.vertices = {{{-0.5f, -0.5f, 0.0f}, color, {0.0f, 0.0f}},
                     {{0.5f, -0.5f, 0.0f}, color, {1.0f, 0.0f}},
                     {{0.5f, 0.5f, 0.0f}, color, {1.0f, 1.0f}},
                     {{-0.5f, 0.5f, 0.0f}, color, {0.0f, 1.0f}}};
    mesh.indices = {0, 1, 2, 2, 3, 0};

    return mesh;
}

template <>
inline Mesh<Vertex> Shape<Cube>::generate(Color const& color) {
    Mesh<Vertex> mesh;
    mesh.vertices.reserve(ShapeTraits<Cube>::vertexCount);
    mesh.indices.reserve(ShapeTraits<Cube>::indexCount);

    // Front
    mesh.vertices.push_back({{-0.5f, -0.5f, 0.5f}, color, {0.0f, 0.0f}});
    mesh.vertices.push_back({{0.5f, -0.5f, 0.5f}, color, {1.0f, 0.0f}});
    mesh.vertices.push_back({{0.5f, 0.5f, 0.5f}, color, {1.0f, 1.0f}});
    mesh.vertices.push_back({{-0.5f, 0.5f, 0.5f}, color, {0.0f, 1.0f}});
    // Back
    mesh.vertices.push_back({{0.5f, -0.5f, -0.5f}, color, {0.0f, 0.0f}});
    mesh.vertices.push_back({{-0.5f, -0.5f, -0.5f}, color, {1.0f, 0.0f}});
    mesh.vertices.push_back({{-0.5f, 0.5f, -0.5f}, color, {1.0f, 1.0f}});
    mesh.vertices.push_back({{0.5f, 0.5f, -0.5f}, color, {0.0f, 1.0f}});
    // Left
    mesh.vertices.push_back({{-0.5f, -0.5f, -0.5f}, color, {0.0f, 0.0f}});
    mesh.vertices.push_back({{-0.5f, -0.5f, 0.5f}, color, {1.0f, 0.0f}});
    mesh.vertices.push_back({{-0.5f, 0.5f, 0.5f}, color, {1.0f, 1.0f}});
    mesh.vertices.push_back({{-0.5f, 0.5f, -0.5f}, color, {0.0f, 1.0f}});
    // Right
    mesh.vertices.push_back({{0.5f, -0.5f, 0.5f}, color, {0.0f, 0.0f}});
    mesh.vertices.push_back({{0.5f, -0.5f, -0.5f}, color, {1.0f, 0.0f}});
    mesh.vertices.push_back({{0.5f, 0.5f, -0.5f}, color, {1.0f, 1.0f}});
    mesh.vertices.push_back({{0.5f, 0.5f, 0.5f}, color, {0.0f, 1.0f}});
    // Top
    mesh.vertices.push_back({{-0.5f, -0.5f, -0.5f}, color, {0.0f, 0.0f}});
    mesh.vertices.push_back({{0.5f, -0.5f, -0.5f}, color, {1.0f, 0.0f}});
    mesh.vertices.push_back({{0.5f, -0.5f, 0.5f}, color, {1.0f, 1.0f}});
    mesh.vertices.push_back({{-0.5f, -0.5f, 0.5f}, color, {0.0f, 1.0f}});
    // Bottom
    mesh.vertices.push_back({{-0.5f, 0.5f, 0.5f}, color, {0.0f, 0.0f}});
    mesh.vertices.push_back({{0.5f, 0.5f, 0.5f}, color, {1.0f, 0.0f}});
    mesh.vertices.push_back({{0.5f, 0.5f, -0.5f}, color, {1.0f, 1.0f}});
    mesh.vertices.push_back({{-0.5f, 0.5f, -0.5f}, color, {0.0f, 1.0f}});

    for (std::uint32_t i = 0; i < 6; i++) {
        std::uint32_t offset = i * 4;
        mesh.indices.push_back(offset + 0);
        mesh.indices.push_back(offset + 1);
        mesh.indices.push_back(offset + 2);
        mesh.indices.push_back(offset + 2);
        mesh.indices.push_back(offset + 3);
        mesh.indices.push_back(offset + 0);
    }

    return mesh;
}

}  // namespace mpvgl
