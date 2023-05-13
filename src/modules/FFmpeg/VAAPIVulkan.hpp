#pragma once

#include <vulkan/VulkanHWInterop.hpp>

#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace QmVk {
class Instance;
class Image;
}
struct AVFrame;
class VAAPI;

class VAAPIVulkan final : public QmVk::HWInterop
{
public:
    VAAPIVulkan();
    ~VAAPIVulkan();

    inline void setVAAPI(const std::shared_ptr<VAAPI> &vaapi)
    {
        m_vaapi = vaapi;
    }
    inline std::shared_ptr<VAAPI> getVAAPI() const
    {
        return m_vaapi;
    }

    QString name() const override;

    void map(Frame &frame) override;
    void clear() override;

    SyncDataPtr sync(const std::vector<Frame> &frames, vk::SubmitInfo *submitInfo = nullptr) override;

public:
    void insertAvailableSurface(uintptr_t id);

private:
    const std::shared_ptr<QmVk::Instance> m_vkInstance;
    std::shared_ptr<VAAPI> m_vaapi;

    bool m_hasDrmFormatModifier = false;

    std::mutex m_mutex;

    std::unordered_set<uintptr_t> m_availableSurfaces;
    std::unordered_map<uintptr_t, std::shared_ptr<QmVk::Image>> m_images;
};
