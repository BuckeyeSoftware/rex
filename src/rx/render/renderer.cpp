#include <string.h>

#include <rx/render/renderer.h>
#include <rx/render/backend.h>
#include <rx/render/backend_gl4.h>

#include <rx/core/filesystem/directory.h>

namespace rx::render {

static constexpr const char* k_technique_path{"base/renderer/techniques"};

renderer::renderer(memory::allocator* _allocator, const char* _backend_name, void* _backend_data)
  : m_allocator{_allocator}
  , m_backend{create_backend(_backend_name, _backend_data)}
  , m_frontend{_allocator, m_backend}
  , m_immediates{nullptr}
{
  // create back buffer and composite buffers
  m_back_target = m_frontend.create_target(RX_RENDER_TAG("backbuffer"));
  m_back_target->request_swapchain();
  m_frontend.initialize_target(RX_RENDER_TAG("backbuffer"), m_back_target);

  m_composite_target = m_frontend.create_target(RX_RENDER_TAG("composite"));
  m_composite_target->request_depth_stencil(texture::data_format::k_d24_s8, {1600, 900});
  m_frontend.initialize_target(RX_RENDER_TAG("composite"), m_composite_target);

  // m_composite_target->request_attachment();

  // read all techniques
  if (filesystem::directory directory{k_technique_path}; directory) {
    directory.each([this](const filesystem::directory::item& _item) {
      if (_item.is_file() && _item.name().ends_with(".json5")) {
        technique new_technique{&m_frontend};
        if (new_technique.load({"%s/%s", k_technique_path, _item.name()})) {
          m_techniques.insert(new_technique.name(), utility::move(new_technique));
        }
      }
    });
  }

  technique* immediate_technique{find_technique_by_name("immediate")};
  RX_ASSERT(immediate_technique, "immediate technique not found");

  m_immediates = reinterpret_cast<immediate*>(m_allocator->allocate(sizeof *m_immediates));
  RX_ASSERT(m_immediates, "out of memory");

  utility::construct<immediate>(m_immediates, &m_frontend, immediate_technique);
}

renderer::~renderer() {
  utility::destruct<immediate>(m_immediates);
  m_allocator->deallocate(reinterpret_cast<rx_byte*>(m_immediates));

  m_frontend.destroy_target(RX_RENDER_TAG("backbuffer"), m_back_target);
  m_frontend.destroy_target(RX_RENDER_TAG("composite"), m_composite_target);

  // process any outstanding rendering
  m_frontend.process();

  utility::destruct<backend>(m_backend);
  m_allocator->deallocate(reinterpret_cast<rx_byte*>(m_backend));
}

bool renderer::update() {
  // clear backbuffer
  m_frontend.clear(
    RX_RENDER_TAG("backbuffer"),
    m_back_target,
    RX_RENDER_CLEAR_COLOR(0),
    {1.0f, 0.0f, 0.0f, 1.0f}
  );

  m_immediates->render(m_back_target);

  // TODO process renderables

  if (m_frontend.process()) {
    m_frontend.swap();
  }

  return m_timer.update();
}

backend* renderer::create_backend(const char* _name, void* _data) {
  if (!strcmp(_name, "gl4")) {
    auto data{m_allocator->allocate(sizeof(backend_gl4))};
    utility::construct<backend_gl4>(data, m_allocator, _data);
    return reinterpret_cast<backend*>(data);
  }
  return nullptr;
}

technique* renderer::find_technique_by_name(const char* _name) {
  return m_techniques.find(_name);
}

} // namespace rx::render