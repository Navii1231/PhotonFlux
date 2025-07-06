#pragma once
#include "vkpch.h"

#define VK_NAMESPACE      vkEngine
#define VK_BEGIN          namespace VK_NAMESPACE {
#define VK_END            }

#define VK_CORE           Core
#define VK_CORE_BEGIN     namespace VK_CORE {
#define VK_CORE_END       }

#define VK_UTILS          Utils
#define VK_UTILS_BEGIN    namespace VK_UTILS {
#define VK_UTILS_END      }
					      
#define VK_ENGINE         VK_NAMESPACE::
#define VK_ENGINE_CORE    VK_CORE::
#define VK_ENGINE_UTILS   VK_UTILS::
