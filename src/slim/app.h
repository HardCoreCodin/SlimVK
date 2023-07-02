#pragma once

#include "./core/base.h"
#include "./vulkan/core/gpu.h"

struct SlimApp {
    timers::Timer update_timer, render_timer;
    bool is_running{true};
    bool is_minimized{false};
    bool blit{false};
    bool suspend_when_minimized{true};

    virtual void OnInit() {};
    virtual void OnShutdown() {};
    virtual void OnWindowMinimized() {};
    virtual void OnWindowRestored() {};
    virtual void OnWindowResize(u16 width, u16 height) {};
    virtual void OnKeyChanged(  u8 key, bool pressed) {};
    virtual void OnMouseButtonUp(  mouse::Button &mouse_button) {};
    virtual void OnMouseButtonDown(mouse::Button &mouse_button) {};
    virtual void OnMouseButtonDoubleClicked(mouse::Button &mouse_button) {};
    virtual void OnMouseWheelScrolled(f32 amount) {};
    virtual void OnMousePositionSet(i32 x, i32 y) {};
    virtual void OnMouseMovementSet(i32 x, i32 y) {};
    virtual void OnMouseRawMovementSet(i32 x, i32 y) {};
    virtual void OnRenderMainPass(GraphicsCommandBuffer &command_buffer) {};
    virtual void OnRender() {
        gpu::beginRenderPass();
        OnRenderMainPass(*gpu::graphics_command_buffer);
        gpu::endRenderPass();
    };
    virtual void OnConfigureMainRenderPass(gpu::RenderPass &main_render_pass) {};
    virtual void OnUpdate(f32 delta_time) {};
    virtual void OnWindowRedraw() {
        update_timer.beginFrame();
        OnUpdate(update_timer.delta_time);
        update_timer.endFrame();

        if (gpu::beginFrame()) {
            render_timer.beginFrame();
            OnRender();
            render_timer.endFrame();
            gpu::endFrame();
        }
    };

    INLINE void _minimize() {
        is_minimized = true;
        OnWindowMinimized();
    }

    INLINE void _restore() {
        is_minimized = false;
        OnWindowRestored();
    }

    INLINE void _resize(u16 width, u16 height) {
        gpu::present::resize(width, height);
        OnWindowResize(width, height);
        OnWindowRedraw();
    }

    INLINE void _redraw() {
        if (suspend_when_minimized && is_minimized)
            return;

        OnWindowRedraw();
    }

    INLINE void _init() {
        gpu::initGPU();
        OnInit();
    }
    INLINE void _mouseButtonUp(  mouse::Button &mouse_button) {};
    INLINE void _mouseButtonDown(mouse::Button &mouse_button) {};
    INLINE void _mouseButtonDoubleClicked(mouse::Button &mouse_button) {};
    INLINE void _mouseWheelScrolled(f32 amount) {};
    INLINE void _mousePositionSet(i32 x, i32 y) {};
    INLINE void _mouseMovementSet(i32 x, i32 y) {};
    INLINE void _mouseRawMovementSet(i32 x, i32 y) {};
    INLINE void _shutdown() {
        gpu::waitForGPU();
        OnShutdown();
        gpu::shutdownGPU();
    }
    INLINE void _keyChanged(u8 key, bool pressed) { OnKeyChanged(key, pressed); }
};

SlimApp* createApp();

#include "./platforms/win32.h"