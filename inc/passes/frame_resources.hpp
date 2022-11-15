#pragma once

#include <data_structures.hpp>

#include <cstdint>

namespace builder {
    enum ImageType {
        NO_IMAGE,
        DEPTH_IMAGE,
        COLOR_IMAGE
    };

    enum Usage {
        Read,
        Write
    };

    struct Dimensions {
        uint32_t width;
        uint32_t height;
        uint32_t depth;

        Dimensions(uint32_t w, uint32_t h, uint32_t d) {
            width = w;
            height = h;
            depth = d;
        }
    };

    class ImageResource {
    public:
        ImageResource();
        
        ImageType type = NO_IMAGE;
        uint32_t id;
        Dimensions dim;
        Usage use;
    };


    class Image {
        friend class pass::FrameBuilder;
    private:
        ImageResource resource_data;
        Image(ImageResource image_resource) {
            resource_data = image_resource;
        }
    };

    class DrawItem {
        std::vector<tuco::Vertex> vertices;
        std::vector<uint32_t> indices;
    };
}