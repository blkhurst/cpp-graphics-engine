#include <blkhurst/integrations/optix_denoiser_pass.hpp>

#ifdef BLKHURST_ENABLE_OPTIX

#include <blkhurst/renderer/render_target.hpp>
#include <blkhurst/textures/texture.hpp>

// clang-format off
// GLAD must be included before CUDA's OpenGL interop header, which includes system gl.h.
#include <glad/gl.h>
#include <cuda_gl_interop.h>
#include <cuda_runtime.h>
#include <optix.h>
#include <optix_stubs.h>
// clang-format on
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace {

constexpr std::size_t kRgba32FBytesPerPixel = sizeof(float) * 4;
constexpr std::size_t kRgba16FBytesPerPixel = sizeof(std::uint16_t) * 4;

void checkCuda(cudaError_t result, const char* label) {
  if (result != cudaSuccess) {
    throw std::runtime_error(std::string(label) + ": " + cudaGetErrorString(result));
  }
}

void checkOptix(OptixResult result, const char* label) {
  if (result != OPTIX_SUCCESS) {
    throw std::runtime_error(std::string(label) + ": " + optixGetErrorName(result) + " (" +
                             std::to_string(static_cast<int>(result)) + ") - " +
                             optixGetErrorString(result));
  }
}

struct ImageLayout {
  std::size_t bytesPerPixel = kRgba16FBytesPerPixel;
  OptixPixelFormat format = OPTIX_PIXEL_FORMAT_HALF4;
};

ImageLayout imageLayoutFor(blkhurst::TextureFormat format) {
  switch (format) {
  case blkhurst::TextureFormat::RGBA16F:
    return {.bytesPerPixel = kRgba16FBytesPerPixel, .format = OPTIX_PIXEL_FORMAT_HALF4};
  case blkhurst::TextureFormat::RGBA32F:
    return {.bytesPerPixel = kRgba32FBytesPerPixel, .format = OPTIX_PIXEL_FORMAT_FLOAT4};
  default:
    throw std::runtime_error("OptixDenoiserPass requires RGBA16F or RGBA32F textures");
  }
}

OptixImage2D makeImage(CUdeviceptr ptr, int width, int height, const ImageLayout& layout) {
  OptixImage2D image{};
  image.data = ptr;
  image.width = static_cast<unsigned>(width);
  image.height = static_cast<unsigned>(height);
  image.rowStrideInBytes = static_cast<std::size_t>(width) * layout.bytesPerPixel;
  image.pixelStrideInBytes = layout.bytesPerPixel;
  image.format = layout.format;
  return image;
}

} // namespace

namespace blkhurst {

struct OptixDenoiserPass::Impl {
  OptixDenoiserPassDesc desc{};
  CUcontext cudaContext = nullptr;
  CUstream stream = nullptr;
  OptixDeviceContext optixContext = nullptr;
  OptixDenoiser denoiser = nullptr;
  OptixDenoiserSizes sizes{};
  CUdeviceptr state = 0;
  CUdeviceptr scratch = 0;
  CUdeviceptr inputImage = 0;
  CUdeviceptr outputImage = 0;
  std::size_t stateSize = 0;
  std::size_t scratchSize = 0;
  std::size_t imageBytes = 0;
  ImageLayout imageLayout{};
  cudaGraphicsResource* inputTextureResource = nullptr;
  cudaGraphicsResource* outputTextureResource = nullptr;
  unsigned inputTextureId = 0;
  unsigned outputTextureId = 0;
  int width = 0;
  int height = 0;

  explicit Impl(const OptixDenoiserPassDesc& initialDesc)
      : desc(initialDesc) {
    if (desc.useAlbedo || desc.useNormal) {
      spdlog::warn("OptixDenoiserPass guide buffers are not implemented yet; ignoring "
                   "albedo/normal guides.");
      desc.useAlbedo = false;
      desc.useNormal = false;
    }

    checkCuda(cudaFree(nullptr), "cudaFree");
    checkOptix(optixInit(), "optixInit");
    checkOptix(optixDeviceContextCreate(cudaContext, nullptr, &optixContext),
               "optixDeviceContextCreate");

    checkCuda(cudaStreamCreate(reinterpret_cast<cudaStream_t*>(&stream)), "cudaStreamCreate");
    createDenoiser();
  }

  ~Impl() {
    unregisterTextures();
    freeImages();
    if (scratch != 0) {
      cudaFree(reinterpret_cast<void*>(scratch));
    }
    if (state != 0) {
      cudaFree(reinterpret_cast<void*>(state));
    }
    if (denoiser != nullptr) {
      optixDenoiserDestroy(denoiser);
    }
    if (stream != nullptr) {
      cudaStreamDestroy(reinterpret_cast<cudaStream_t>(stream));
    }
    if (optixContext != nullptr) {
      optixDeviceContextDestroy(optixContext);
    }
  }

  void createDenoiser() {
    OptixDenoiserOptions options{};
    options.guideAlbedo = desc.useAlbedo ? 1U : 0U;
    options.guideNormal = desc.useNormal ? 1U : 0U;
    options.denoiseAlpha = OPTIX_DENOISER_ALPHA_MODE_COPY;
    checkOptix(
        optixDenoiserCreate(optixContext, OPTIX_DENOISER_MODEL_KIND_HDR, &options, &denoiser),
        "optixDenoiserCreate");
  }

  void resize(int width, int height) {
    this->width = width;
    this->height = height;
    if (width <= 0 || height <= 0) {
      return;
    }

    checkOptix(optixDenoiserComputeMemoryResources(denoiser, static_cast<unsigned>(width),
                                                   static_cast<unsigned>(height), &sizes),
               "optixDenoiserComputeMemoryResources");

    stateSize = sizes.stateSizeInBytes;
    scratchSize =
        std::max(sizes.withoutOverlapScratchSizeInBytes, sizes.withOverlapScratchSizeInBytes);

    if (state != 0) {
      cudaFree(reinterpret_cast<void*>(state));
    }
    if (scratch != 0) {
      cudaFree(reinterpret_cast<void*>(scratch));
    }

    checkCuda(cudaMalloc(reinterpret_cast<void**>(&state), stateSize), "cudaMalloc(state)");
    checkCuda(cudaMalloc(reinterpret_cast<void**>(&scratch), scratchSize), "cudaMalloc(scratch)");

    checkOptix(optixDenoiserSetup(denoiser, stream, static_cast<unsigned>(width),
                                  static_cast<unsigned>(height), state, stateSize, scratch,
                                  scratchSize),
               "optixDenoiserSetup");

    resizeImages(width, height);
  }

  void freeImages() {
    if (inputImage != 0) {
      cudaFree(reinterpret_cast<void*>(inputImage));
      inputImage = 0;
    }
    if (outputImage != 0) {
      cudaFree(reinterpret_cast<void*>(outputImage));
      outputImage = 0;
    }
  }

  void resizeImages(int width, int height) {
    imageBytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) *
                 imageLayout.bytesPerPixel;

    freeImages();
    checkCuda(cudaMalloc(reinterpret_cast<void**>(&inputImage), imageBytes),
              "cudaMalloc(inputImage)");
    checkCuda(cudaMalloc(reinterpret_cast<void**>(&outputImage), imageBytes),
              "cudaMalloc(outputImage)");
  }

  void unregisterTextures() {
    if (inputTextureResource != nullptr) {
      cudaGraphicsUnregisterResource(inputTextureResource);
      inputTextureResource = nullptr;
    }
    if (outputTextureResource != nullptr) {
      cudaGraphicsUnregisterResource(outputTextureResource);
      outputTextureResource = nullptr;
    }
    inputTextureId = 0;
    outputTextureId = 0;
  }

  void registerTextures(const Texture& input, const Texture& output) {
    if (inputTextureId == input.id() && outputTextureId == output.id()) {
      return;
    }

    unregisterTextures();
    imageLayout = imageLayoutFor(input.desc().format);
    if (output.desc().format != input.desc().format) {
      throw std::runtime_error("OptixDenoiserPass input/output texture formats must match");
    }
    resizeImages(width, height);

    checkCuda(cudaGraphicsGLRegisterImage(&inputTextureResource, input.id(), GL_TEXTURE_2D,
                                          cudaGraphicsRegisterFlagsReadOnly),
              "cudaGraphicsGLRegisterImage(input)");
    checkCuda(cudaGraphicsGLRegisterImage(&outputTextureResource, output.id(), GL_TEXTURE_2D,
                                          cudaGraphicsRegisterFlagsWriteDiscard),
              "cudaGraphicsGLRegisterImage(output)");
    inputTextureId = input.id();
    outputTextureId = output.id();
  }

  void denoiseImages() {
    OptixDenoiserParams params{};

    OptixDenoiserLayer layer{};
    layer.input = makeImage(inputImage, width, height, imageLayout);
    layer.output = makeImage(outputImage, width, height, imageLayout);

    OptixDenoiserGuideLayer guide{};
    checkOptix(optixDenoiserInvoke(denoiser, stream, &params, state, stateSize, &guide, &layer, 1,
                                   0, 0, scratch, scratchSize),
               "optixDenoiserInvoke");
  }

  void denoiseTextureToTexture(const Texture& input, const Texture& output) {
    if (width <= 0 || height <= 0) {
      return;
    }
    registerTextures(input, output);

    cudaGraphicsResource* resources[] = {inputTextureResource, outputTextureResource};
    auto* cudaStream = reinterpret_cast<cudaStream_t>(stream);
    checkCuda(cudaGraphicsMapResources(2, resources, cudaStream), "cudaGraphicsMapResources");

    cudaArray_t inputArray = nullptr;
    cudaArray_t outputArray = nullptr;
    checkCuda(cudaGraphicsSubResourceGetMappedArray(&inputArray, inputTextureResource, 0, 0),
              "cudaGraphicsSubResourceGetMappedArray(input)");
    checkCuda(cudaGraphicsSubResourceGetMappedArray(&outputArray, outputTextureResource, 0, 0),
              "cudaGraphicsSubResourceGetMappedArray(output)");

    const std::size_t rowBytes = static_cast<std::size_t>(width) * imageLayout.bytesPerPixel;
    checkCuda(cudaMemcpy2DFromArrayAsync(reinterpret_cast<void*>(inputImage), rowBytes, inputArray,
                                         0, 0, rowBytes, static_cast<std::size_t>(height),
                                         cudaMemcpyDeviceToDevice, cudaStream),
              "cudaMemcpy2DFromArrayAsync(input)");

    denoiseImages();

    checkCuda(cudaMemcpy2DToArrayAsync(outputArray, 0, 0, reinterpret_cast<void*>(outputImage),
                                       rowBytes, rowBytes, static_cast<std::size_t>(height),
                                       cudaMemcpyDeviceToDevice, cudaStream),
              "cudaMemcpy2DToArrayAsync(output)");
    checkCuda(cudaGraphicsUnmapResources(2, resources, cudaStream), "cudaGraphicsUnmapResources");
  }
};

OptixDenoiserPass::OptixDenoiserPass(const OptixDenoiserPassDesc& desc) {
  try {
    impl_ = new Impl(desc);
  } catch (const std::exception& e) {
    spdlog::error("OptixDenoiserPass disabled: {}", e.what());
    setEnabled(false);
  }
}

std::shared_ptr<OptixDenoiserPass> OptixDenoiserPass::create(const OptixDenoiserPassDesc& desc) {
  return std::make_shared<OptixDenoiserPass>(desc);
}

OptixDenoiserPass::~OptixDenoiserPass() {
  delete impl_;
}

void OptixDenoiserPass::setSize(int width, int height) {
  Pass::setSize(width, height);
  if (impl_ == nullptr) {
    return;
  }
  impl_->resize(width, height);
}

void OptixDenoiserPass::render(const PassRenderContext& context) {
  if (!isEnabled() || impl_ == nullptr) {
    return;
  }
  if (context.renderToScreen) {
    spdlog::warn(
        "OptixDenoiserPass requires a following OutputPass to present the denoised result.");
  }
  if (context.readBuffer == nullptr || context.writeBuffer == nullptr ||
      !context.readBuffer->texture() || !context.writeBuffer->texture()) {
    return;
  }

  try {
    impl_->denoiseTextureToTexture(*context.readBuffer->texture(), *context.writeBuffer->texture());
  } catch (const std::exception& e) {
    spdlog::error("OptixDenoiserPass disabled: {}", e.what());
    setEnabled(false);
  }
}

} // namespace blkhurst

#endif // BLKHURST_ENABLE_OPTIX
