#include <bedrock/image.hpp>
#include <logger/interface.hpp>

#include <antuco.hpp>
#include <api_graphics.hpp>

#include <data_structures.hpp>
#include <memory_allocator.hpp>

#include <stb_image.h>

using namespace br;

#define CUBEMAP_IMAGE_COUNT 6


void Image::destroy()
{
	INFO("image name {}", data.name.c_str());
	destroy_image_view();
	if (image != VK_NULL_HANDLE)
	{
		device->get().destroyImage(image, nullptr);
	}
	if (memory != VK_NULL_HANDLE)
	{
		device->get().freeMemory(memory, nullptr);
	}

	if (sampler != VK_NULL_HANDLE)
	{
		device->get().destroySampler(sampler, nullptr);
	}

	// invalidate handles
	image = VK_NULL_HANDLE;
	memory = VK_NULL_HANDLE;
}

void Image::destroy_image_view()
{
	for (auto& image_view : image_views)
	{
		if (image_view != VK_NULL_HANDLE)
		{
			device->get().destroyImageView(image_view, nullptr);
		}
		image_view = VK_NULL_HANDLE;
	}
	if (command_pool != VK_NULL_HANDLE)
	{
		device->get().destroyCommandPool(command_pool);
	}

	// invalidate handles
	command_pool = VK_NULL_HANDLE;
}

vk::Image Image::get_api_image() { return image; }

void Image::init(std::string name, bool handle_destruction)
{
	image_views.reserve(MAX_VIEWS);
	view_index = 0;
	sampler = VK_NULL_HANDLE;
	Image::handle_destruction = handle_destruction;
	data.name = name;

	// Get backend.
	tuco::GraphicsImpl* backend = tuco::Antuco::get_engine().get_backend();

	Image::device = backend->p_device;
	Image::p_physical_device = backend->p_physical_device;

	auto pool_info = vk::CommandPoolCreateInfo({}, device->get_transfer_family());
	command_pool = device->get().createCommandPool(pool_info);

	initialized = true;
}

void Image::load_blank(ImageDetails info, uint32_t width, uint32_t height, uint32_t layers)
{
	uint32_t channels;
	uint32_t size;
	vk::Format v_format = get_vk_format(info.format, channels, size);

	ImageCreateInfo image_info;
	image_info.format = v_format;
	image_info.extent = vk::Extent3D(width, height, 1);
	image_info.usage = get_vk_usage(info.format, info.usage);
	//image_info.initial_layout = vk::ImageLayout::eTransferDstOptimal;
	image_info.queueFamilyIndexCount = 1;
	image_info.pQueueFamilyIndices = &device->get_graphics_family();
	image_info.size = width * height * channels * size;
	image_info.image_type = vk::ImageType::e2D;
	image_info.arrayLayers = layers;
	if (info.type == ImageType::Cube) image_info.flags = vk::ImageCreateFlagBits::eCubeCompatible;

	data.image_info = image_info;

	create_image();
}

void Image::create_view(uint32_t layer_count, uint32_t base_layer, ImageType type)
{
	ImageViewCreateInfo info{};
	info.base_array_layer = base_layer;
	info.layer_count = layer_count;
	if ((data.image_info.usage & vk::ImageUsageFlagBits::eColorAttachment) == vk::ImageUsageFlagBits::eColorAttachment)
	{
		info.aspect_mask = vk::ImageAspectFlagBits::eColor;
	}
	if ((data.image_info.usage & vk::ImageUsageFlagBits::eDepthStencilAttachment) == vk::ImageUsageFlagBits::eDepthStencilAttachment)
	{
		info.aspect_mask = vk::ImageAspectFlagBits::eDepth;
	}

	info.view_type = type == ImageType::Cube ? vk::ImageViewType::eCube : vk::ImageViewType::e2D;

	data.image_view_info = info;

	create_image_view();
}

void Image::set_image_sampler(VkFilter filter, VkSamplerMipmapMode mipMapFilter, VkSamplerAddressMode addressMode)
{
	VkSamplerCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = {};
	info.magFilter = filter;
	info.minFilter = filter;
	info.mipmapMode = mipMapFilter;
	info.addressModeU = addressMode;
	info.addressModeV = addressMode;
	info.addressModeW = addressMode;
	info.mipLodBias = 0.f; // IDK
	info.anisotropyEnable = VK_FALSE; // IDK
	info.compareEnable = VK_FALSE; // IDK
	info.unnormalizedCoordinates = VK_FALSE;

	sampler = device->get().createSampler(info);

	ASSERT(sampler, "Failed to create image sampler.");
}

bool Image::is_3d_image(ImageType type)
{
	return type == ImageType::Image_3D;
}

vk::ImageUsageFlags Image::get_vk_usage(ImageFormat format, ImageUsage usage)
{
	if (format == ImageFormat::DEPTH && usage == ImageUsage::RENDER_OUTPUT)
	{
		return vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
	}
	else if (usage == ImageUsage::RENDER_OUTPUT)
	{
		return vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
	}
	else if (usage == ImageUsage::SHADER_INPUT)
	{
		return vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
	}

	ASSERT(false, "could not find VkUsage for described image");
	return vk::ImageUsageFlagBits::eSampled;
}

vk::Format Image::get_vk_format(ImageFormat image_format, uint32_t& channels, uint32_t& size)
{
	switch (image_format)
	{
	case ImageFormat::RGBA_COLOR:
		channels = 4;
		size = 1;
		return vk::Format::eR8G8B8A8Srgb;
	case ImageFormat::HDR_COLOR:
		channels = 4;
		size = 4;
		return vk::Format::eR32G32B32A32Sfloat;
	case ImageFormat::DEPTH:
		channels = 1;
		size = 4;
		return vk::Format::eD16Unorm;
	case ImageFormat::R_COLOR:
		channels = 1;
		size = 1;
		return vk::Format::eR8Srgb;
	case ImageFormat::RG_COLOR:
		channels = 2;
		size = 1;
		return vk::Format::eR8G8Srgb;
	default:
		break;
	}

	ERR("Failed to map format {} to vk type.", static_cast<uint32_t>(image_format));
	channels = 0;
	return vk::Format::eUndefined;
}

// REQUIRES: every image in cube_images should be the same size.
void Image::load_cubemap(std::vector<std::string>& cube_images, ImageFormat image_format)
{
	uint32_t channels;
	uint32_t size;
	vk::Format format = get_vk_format(image_format, channels, size);

	// load images
	raw_image.image_count = CUBEMAP_IMAGE_COUNT;
	raw_image.images.resize(CUBEMAP_IMAGE_COUNT);
	for (int i = 0; i < CUBEMAP_IMAGE_COUNT; i++)
	{
		raw_image.images[i] = stbi_load(cube_images[i].c_str(), &raw_image.width, &raw_image.height, nullptr, channels);
		raw_image.channels = channels;
	}

	raw_image.image_size = raw_image.width * raw_image.height * raw_image.channels;
	raw_image.buffer_size = raw_image.image_size * raw_image.image_count;


	mem::CPUBuffer buffer;
	init_buffer(raw_image.buffer_size, &buffer);
	uint32_t offset = 0;
	for (int i = 0; i < CUBEMAP_IMAGE_COUNT; i++)
	{
		offset = add_to_buffer(raw_image, i, offset, &buffer);
	}

	ImageCreateInfo image_info;
	image_info.format = format;
	image_info.extent = vk::Extent3D(raw_image.width, raw_image.height, 1);
	image_info.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
	//image_info.initial_layout = vk::ImageLayout::eTransferDstOptimal;
	image_info.queueFamilyIndexCount = 1;
	image_info.pQueueFamilyIndices = &device->get_graphics_family();
	image_info.size = raw_image.image_size;
	image_info.image_type = vk::ImageType::e2D;
	image_info.arrayLayers = CUBEMAP_IMAGE_COUNT;
	image_info.flags = vk::ImageCreateFlagBits::eCubeCompatible;

	ImageViewCreateInfo view_info;
	view_info.aspect_mask = vk::ImageAspectFlagBits::eColor;
	view_info.view_type = vk::ImageViewType::eCube;
	view_info.layer_count = CUBEMAP_IMAGE_COUNT;

	data.image_info = image_info;
	data.image_view_info = view_info;

	create_image();
	create_image_view();

	transfer(vk::ImageLayout::eTransferDstOptimal, device->get_transfer_queue());

	copy_from_buffer(buffer.get(), vk::Offset3D(), CUBEMAP_IMAGE_COUNT, raw_image.image_size, vk::Extent3D(raw_image.width, raw_image.height, 1), device->get_transfer_queue());

	transfer(vk::ImageLayout::eShaderReadOnlyOptimal, device->get_transfer_queue(), std::nullopt, vk::ImageLayout::eTransferDstOptimal);

	current_layout = vk::ImageLayout::eShaderReadOnlyOptimal;

	buffer.destroy();
}

void Image::load_image(std::string& file_path, ImageFormat image_format, ImageType type)
{
	uint32_t channels;
	uint32_t size;
	vk::Format format = get_vk_format(image_format, channels, size);

	raw_image.image_count = 1;
	raw_image.images.resize(raw_image.image_count);
	raw_image.images[0] = stbi_load(file_path.c_str(), &raw_image.width, &raw_image.height, nullptr, channels);
	raw_image.channels = channels;
	raw_image.size = size;
	raw_image.buffer_size = raw_image.width * raw_image.height * raw_image.channels * raw_image.size;
	raw_image.image_size = raw_image.width * raw_image.height * raw_image.channels * raw_image.size;

	load_to_gpu(format, type);
}

void Image::load_float_image(std::string& file_path, ImageFormat image_format, ImageType type)
{
	uint32_t channels;
	uint32_t size;
	vk::Format format = get_vk_format(image_format, channels, size);

	raw_image.image_count = 1;
	raw_image.images.resize(raw_image.image_count);
	raw_image.images[0] = reinterpret_cast<unsigned char*>(stbi_loadf(file_path.c_str(), &raw_image.width, &raw_image.height, nullptr, channels));
	raw_image.channels = channels;
	raw_image.size = size;
	raw_image.buffer_size = raw_image.width * raw_image.height * raw_image.channels * raw_image.size;
	raw_image.image_size = raw_image.width * raw_image.height * raw_image.channels * raw_image.size;

	load_to_gpu(format, type);
}

// REQUIRES: Raw image should have already been loaded in.
void Image::load_to_gpu(vk::Format format, ImageType type)
{

	// Load image to CPU buffer.
	mem::CPUBuffer buffer;
	init_buffer(raw_image.buffer_size, &buffer);
	add_to_buffer(raw_image, 0, 0, &buffer);

	// TODO - embed image info into the image itself (should be some library to extract info). simplify image creation and make it more universal.
	// Initialize device image.
	ImageCreateInfo image_info;

	// [TODO - 08/2024] - Image Array Support.
	image_info.format = format;
	image_info.extent = vk::Extent3D(raw_image.width, raw_image.height, 1);
	image_info.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
	//image_info.initial_layout = vk::ImageLayout::eTransferDstOptimal;
	image_info.queueFamilyIndexCount = 1;
	image_info.pQueueFamilyIndices = &device->get_graphics_family();
	image_info.size = raw_image.image_size;
	image_info.image_type = is_3d_image(type) ? vk::ImageType::e3D : vk::ImageType::e2D;

	// Generalize to support other non-color type images.
	ImageViewCreateInfo view_info;
	view_info.aspect_mask = vk::ImageAspectFlagBits::eColor;
	view_info.view_type = is_3d_image(type) ? vk::ImageViewType::e3D : vk::ImageViewType::e2D;

	// TODO - add support to cleaning up image if user wants to change material texture.
	data.image_info = image_info;
	data.image_view_info = view_info;
	create_image();
	create_image_view();

	transfer(vk::ImageLayout::eTransferDstOptimal, device->get_transfer_queue());

	copy_from_buffer(buffer.get(), vk::Offset3D(), 1, raw_image.image_size, vk::Extent3D(raw_image.width, raw_image.height, 1), device->get_transfer_queue());

	transfer(vk::ImageLayout::eShaderReadOnlyOptimal, device->get_transfer_queue(), std::nullopt, vk::ImageLayout::eTransferDstOptimal);

	current_layout = vk::ImageLayout::eShaderReadOnlyOptimal;

	buffer.destroy();
}

void Image::load_color_image(std::string file_path)
{
	// Load raw image
	raw_image.image_count = 1;
	raw_image.images.resize(raw_image.image_count);
	raw_image.images[0] = stbi_load(file_path.c_str(), &raw_image.width, &raw_image.height, nullptr, 4);
	raw_image.channels = 4;
	raw_image.buffer_size = raw_image.width * raw_image.height * raw_image.channels;
	raw_image.image_size = raw_image.width * raw_image.height * raw_image.channels;

	// Load image to CPU buffer.
	mem::CPUBuffer buffer;
	init_buffer(raw_image.buffer_size, &buffer);
	add_to_buffer(raw_image, 0, 0, &buffer);
	//copy_to_buffer(raw_image, 0, &buffer);

	// TODO - embed image info into the image itself (should be some library to extract info). simplify image creation and make it more universal.
	// Initialize device image.
	ImageCreateInfo image_info;

	// TODO : take format info from user.
	if (raw_image.channels == 4)
	{
		image_info.format = vk::Format::eR8G8B8A8Srgb;
	}
	else if (raw_image.channels == 3)
	{
		image_info.format = vk::Format::eR8G8B8Srgb;
	}
	else
	{
		ERR("Engine only supports images with channel count of 3 or 4, this image has channel count of [%d]", raw_image.channels);
	}
	image_info.extent = vk::Extent3D(raw_image.width, raw_image.height, 1);
	image_info.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
	//image_info.initial_layout = vk::ImageLayout::eTransferDstOptimal;
	image_info.queueFamilyIndexCount = 1;
	image_info.pQueueFamilyIndices = &device->get_graphics_family();
	image_info.size = raw_image.image_size;

	// Generalize to support other non-color type images.
	ImageViewCreateInfo view_info;
	view_info.aspect_mask = vk::ImageAspectFlagBits::eColor;

	// TODO - add support to cleaning up image if user wants to change material texture.
	data.image_info = image_info;
	data.image_view_info = view_info;
	create_image();
	create_image_view();

	transfer(vk::ImageLayout::eTransferDstOptimal, device->get_transfer_queue());

	copy_from_buffer(buffer.get(), vk::Offset3D(), 1, raw_image.image_size, vk::Extent3D(raw_image.width, raw_image.height, 1), device->get_transfer_queue());

	transfer(vk::ImageLayout::eShaderReadOnlyOptimal, device->get_transfer_queue(), std::nullopt, vk::ImageLayout::eTransferDstOptimal);

	current_layout = vk::ImageLayout::eShaderReadOnlyOptimal;

	buffer.destroy();
}

void Image::init_buffer(uint32_t buffer_size, mem::CPUBuffer* buffer)
{
	// create cpu buffer
	mem::BufferCreateInfo texture_buffer_info{};
	texture_buffer_info.size = buffer_size;
	texture_buffer_info.usage = vk::BufferUsageFlagBits::eTransferSrc;
	texture_buffer_info.sharing_mode = vk::SharingMode::eExclusive;
	texture_buffer_info.queue_family_index_count = 1;
	texture_buffer_info.p_queue_family_indices = &device->get_transfer_family();

	// TODO: make buffer at runtime specifically for transfer commands
	buffer->init(*p_physical_device, *device, texture_buffer_info);
}

uint32_t Image::add_to_buffer(RawImageData& data, uint32_t image_index, uint32_t offset, mem::CPUBuffer* buffer)
{
	buffer->map(data.image_size, offset, data.images[image_index]);
	return offset + data.image_size;
}

void Image::init(std::shared_ptr<v::PhysicalDevice> p_physical_device, std::shared_ptr<v::Device> device,
				 VkImage image, ImageData info, bool handle_destruction)
{
	image_views.reserve(MAX_VIEWS);
	view_index = 0;
	sampler = VK_NULL_HANDLE;
	Image::handle_destruction = handle_destruction;
	data = info;

	Image::device = device;
	Image::p_physical_device = p_physical_device;

	Image::image = image;
	create_image_view();

	current_layout = data.image_info.initial_layout;

	// create command pool
	auto pool_info = vk::CommandPoolCreateInfo({}, device->get_transfer_family());

	command_pool = device->get().createCommandPool(pool_info);
	initialized = true;
}

vk::ImageView Image::get_api_image_view(uint32_t index) { return image_views[index]; }

void Image::init(std::shared_ptr<v::PhysicalDevice> p_physical_device, std::shared_ptr<v::Device> device,
				 ImageData info, bool handle_destruction)
{
	image_views.reserve(MAX_VIEWS);
	view_index = 0;
	sampler = VK_NULL_HANDLE;
	Image::handle_destruction = handle_destruction;
	Image::device = device;
	Image::p_physical_device = p_physical_device;
	data = info;

	create_image();
	create_image_view();

	current_layout = data.image_info.initial_layout;

	auto pool_info = vk::CommandPoolCreateInfo({}, device->get_transfer_family());

	command_pool = device->get().createCommandPool(pool_info);

	initialized = true;
}

Image::~Image()
{
	if (handle_destruction || !initialized)
	{
		return;
	}

	destroy();
}

void Image::create_image()
{
	auto info = data.image_info;
	auto image_info = vk::ImageCreateInfo(
		info.flags, info.image_type, info.format, info.extent, info.mipLevels,
		info.arrayLayers, info.samples, info.tiling, info.usage, info.sharingMode,
		info.queueFamilyIndexCount, info.pQueueFamilyIndices,
		info.initial_layout);

	image = device->get().createImage(image_info);

	// allocate memory
	auto memory_req = device->get().getImageMemoryRequirements(image);
	auto memory_prop = p_physical_device->get().getMemoryProperties();

	auto memory_index = uint32_t(0);
	auto properties = info.memory_properties;
	// uint32_t suitableMemoryForBuffer = 0;
	for (uint32_t i = 0; i < memory_prop.memoryTypeCount; i++)
	{
		if (memory_req.memoryTypeBits & (1 << i) &&
			((memory_prop.memoryTypes[i].propertyFlags & properties) ==
			 properties))
		{
			memory_index = i;
			break;
		}
	}

	auto alloc = vk::MemoryAllocateInfo(memory_req.size, memory_index);

	alloc.memoryTypeIndex = mem::findMemoryType(
		*p_physical_device, memory_req.memoryTypeBits, info.memory_properties);

	memory = device->get().allocateMemory(alloc);

	device->get().bindImageMemory(image, memory, 0);
}

///
/// transfers data from a buffer to an image
/// PARAMETERS
///     - mem::Memory buffer : the source buffer that the data will be
///     transferred from
///     - mem::Memory image : the image that the data will be transferred to
///     - VkOffset3D image_offset : which part of the image the buffer data will
///     be mapped to
///     - VkExtent3D map_size : describes the size of the image that the buffer
///     is copying,
//                              if null, will use default size of image
/// RETURNS - VOID
void Image::copy_from_buffer(vk::Buffer buffer, vk::Offset3D image_offset, uint32_t image_count, uint32_t image_size,
							 std::optional<vk::Extent3D> map_size,
							 vk::Queue queue,
							 std::optional<vk::CommandBuffer> command_buffer)
{
// create command buffer
	bool delete_buffer = false;
	if (!command_buffer.has_value())
	{
		command_buffer = tuco::begin_command_buffer(*device, command_pool);
		delete_buffer = true;
	}

	//auto copy = vk::BufferImageCopy(
	//    static_cast<vk::DeviceSize>(0), 0, 0, image_layers, image_offset,
	//    map_size.has_value() ? map_size.value() : data.image_info.extent);

	std::vector<vk::BufferImageCopy> copy_regions;
	vk::Extent3D extent = map_size.has_value() ? map_size.value() : data.image_info.extent;
	for (int i = 0; i < image_count; i++)
	{
		auto image_layers = vk::ImageSubresourceLayers(
			data.image_view_info.aspect_mask, data.image_view_info.base_mip_level,
			i, 1);

		uint32_t offset = i * image_size;
		copy_regions.push_back(vk::BufferImageCopy(offset, 0, 0, image_layers, image_offset, extent));
	}

	command_buffer.value().copyBufferToImage(
		buffer, image, vk::ImageLayout::eTransferDstOptimal, image_count, copy_regions.data());

	if (delete_buffer)
	{
		tuco::end_command_buffer(*device, queue, command_pool,
								 command_buffer.value());
	}
}

void Image::change_layout(vk::ImageLayout new_layout, vk::Queue queue, std::optional<vk::CommandBuffer> command_buffer)
{
	if (current_layout == new_layout)
	{
		//INFO("Image %s already in layout %d", data.name, new_layout);
		return; // already changed layout.
	}

	transfer(new_layout, queue, command_buffer, current_layout);
	current_layout = new_layout;
}

// [TODO] - WHY THE FUCK DID I WRITE THIS LIKE THIS?
void Image::transfer(vk::ImageLayout output_layout, vk::Queue queue,
					 std::optional<vk::CommandBuffer> command_buffer,
					 vk::ImageLayout current_layout)
{
// begin command buffer
	bool delete_buffer = false;
	if (!command_buffer.has_value())
	{
		command_buffer = tuco::begin_command_buffer(*device, command_pool);
		delete_buffer = true;
	}

	vk::ImageLayout initial_layout = data.image_info.initial_layout;
	if (current_layout != vk::ImageLayout::eUndefined)
		initial_layout = current_layout;
	data.image_info.initial_layout = output_layout;

	auto src_access = vk::AccessFlagBits();
	auto dst_access = vk::AccessFlagBits();

	auto src_stage = vk::PipelineStageFlagBits();
	auto dst_stage = vk::PipelineStageFlagBits();

	if (output_layout == vk::ImageLayout::eTransferSrcOptimal &&
		initial_layout == vk::ImageLayout::eColorAttachmentOptimal)
	{
		src_access = vk::AccessFlagBits::eColorAttachmentWrite;
		dst_access = vk::AccessFlagBits::eTransferRead;

		src_stage = vk::PipelineStageFlagBits::eTransfer;
		dst_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	}
	else if (output_layout == vk::ImageLayout::eColorAttachmentOptimal &&
			 initial_layout == vk::ImageLayout::eTransferSrcOptimal)
	{
		src_access = vk::AccessFlagBits::eTransferWrite;
		dst_access = vk::AccessFlagBits::eColorAttachmentWrite;

		src_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dst_stage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (output_layout == vk::ImageLayout::eShaderReadOnlyOptimal &&
			 initial_layout == vk::ImageLayout::eColorAttachmentOptimal)
	{
		src_access = vk::AccessFlagBits::eColorAttachmentWrite;
		dst_access = vk::AccessFlagBits::eShaderRead;

		src_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (output_layout == vk::ImageLayout::eColorAttachmentOptimal &&
			 initial_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		src_access = vk::AccessFlagBits::eShaderRead;
		dst_access = vk::AccessFlagBits::eColorAttachmentWrite;

		src_stage = vk::PipelineStageFlagBits::eFragmentShader;
		dst_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	}
	else if (output_layout == vk::ImageLayout::ePresentSrcKHR &&
			 initial_layout == vk::ImageLayout::eUndefined)
	{
		src_access = {};
		dst_access = {};

		src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
		dst_stage = vk::PipelineStageFlagBits::eBottomOfPipe;
	}
	else if (output_layout == vk::ImageLayout::eTransferDstOptimal &&
			 initial_layout == vk::ImageLayout::ePresentSrcKHR)
	{
		src_access = vk::AccessFlagBits::eColorAttachmentRead;
		dst_access = vk::AccessFlagBits::eTransferWrite;

		src_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dst_stage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (output_layout == vk::ImageLayout::ePresentSrcKHR &&
			 initial_layout == vk::ImageLayout::eTransferDstOptimal)
	{
		src_access = vk::AccessFlagBits::eTransferWrite;
		dst_access = {};

		src_stage = vk::PipelineStageFlagBits::eTransfer;
		dst_stage = vk::PipelineStageFlagBits::eBottomOfPipe;
	}
	else if (output_layout == vk::ImageLayout::eTransferDstOptimal &&
			 initial_layout == vk::ImageLayout::eUndefined)
	{
		src_access = {};
		dst_access = vk::AccessFlagBits::eTransferWrite;

		src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
		dst_stage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (output_layout == vk::ImageLayout::eTransferDstOptimal &&
			 initial_layout == vk::ImageLayout::eTransferSrcOptimal)
	{
		src_access = vk::AccessFlagBits::eShaderRead;
		dst_access = vk::AccessFlagBits::eTransferWrite;

		src_stage = vk::PipelineStageFlagBits::eFragmentShader;
		dst_stage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (output_layout == vk::ImageLayout::eShaderReadOnlyOptimal &&
			 initial_layout == vk::ImageLayout::eTransferDstOptimal)
	{
		src_access = vk::AccessFlagBits::eTransferWrite;
		dst_access = vk::AccessFlagBits::eShaderRead;

		src_stage = vk::PipelineStageFlagBits::eTransfer;
		dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (output_layout == vk::ImageLayout::eShaderReadOnlyOptimal &&
			 initial_layout ==
			 vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		src_access = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		dst_access = vk::AccessFlagBits::eShaderRead;

		src_stage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (output_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal &&
			 initial_layout == vk::ImageLayout::eUndefined)
	{
		src_access = {};
		dst_access = vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
		dst_stage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	}
	else if (output_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal &&
			 initial_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		src_access = vk::AccessFlagBits::eShaderRead;
		dst_access = vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		src_stage = vk::PipelineStageFlagBits::eFragmentShader;
		dst_stage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	}
	else if (output_layout == vk::ImageLayout::eShaderReadOnlyOptimal &&
			 initial_layout == vk::ImageLayout::eUndefined)
	{
		src_access = {};
		dst_access = vk::AccessFlagBits::eShaderRead;

		src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
		dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (output_layout == vk::ImageLayout::eDepthStencilReadOnlyOptimal &&
			 initial_layout ==
			 vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		src_access = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		dst_access = vk::AccessFlagBits::eDepthStencilAttachmentRead;

		src_stage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		dst_stage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	}
	else if (output_layout == vk::ImageLayout::eTransferSrcOptimal &&
			 initial_layout ==
			 vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		src_access = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		dst_access = vk::AccessFlagBits::eTransferRead;

		src_stage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		dst_stage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (output_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal &&
			 initial_layout == vk::ImageLayout::eTransferSrcOptimal)
	{
		src_access = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		dst_access = vk::AccessFlagBits::eTransferRead;

		src_stage = vk::PipelineStageFlagBits::eTransfer;
		dst_stage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	}
	else if (output_layout == vk::ImageLayout::eTransferSrcOptimal &&
			 initial_layout == vk::ImageLayout::eDepthStencilReadOnlyOptimal)
	{
		src_access = vk::AccessFlagBits::eDepthStencilAttachmentRead;
		dst_access = vk::AccessFlagBits::eTransferRead;

		src_stage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		dst_stage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (output_layout == vk::ImageLayout::eDepthStencilReadOnlyOptimal &&
			 initial_layout == vk::ImageLayout::eTransferSrcOptimal)
	{
		src_access = vk::AccessFlagBits::eDepthStencilAttachmentRead;
		dst_access = vk::AccessFlagBits::eTransferRead;

		src_stage = vk::PipelineStageFlagBits::eTransfer;
		dst_stage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	}

	auto subresource =
		vk::ImageSubresourceRange(data.image_view_info.aspect_mask, 0, data.image_info.mipLevels, 0, data.image_info.arrayLayers);

	auto image_transfer = vk::ImageMemoryBarrier(
		src_access, dst_access, initial_layout, output_layout,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, image, subresource);

	command_buffer.value().pipelineBarrier(
		src_stage, dst_stage, vk::DependencyFlagBits::eByRegion, 0, nullptr, 0,
		nullptr, 1, &image_transfer);
	// end command buffer
	if (delete_buffer)
	{
		tuco::end_command_buffer(*device, queue, command_pool,
								 command_buffer.value());
	}
}

void Image::create_image_view()
{	
	auto info = data.image_view_info;
	auto format = data.image_info.format;
	if (info.format.has_value())
		format = info.format.value();

	auto components = vk::ComponentMapping(info.r, info.g, info.b, info.a);

	auto resource_range = vk::ImageSubresourceRange(
		info.aspect_mask, info.base_mip_level, info.level_count,
		info.base_array_layer, info.layer_count);

	auto create_info = vk::ImageViewCreateInfo({}, image, info.view_type, format,
											   components, resource_range);

	
	ASSERT(image_views.size() < MAX_VIEWS, "Attempting to allocate image index: {} which is more than the allowed images. Please increase the MAX_VIEWS value.", image_views.size());
	image_views.push_back(device->get().createImageView(create_info));
	view_index++;
}