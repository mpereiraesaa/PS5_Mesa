/*
 * Copyright © 2026 Manuel Pereira
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * Where a VideoOut swapchain's images go (wsi_common_videoout.c), apart from
 * the rest of the backend so a host test can check it on its own.
 *
 * VideoOut takes framebuffers of a few sizes only. On FW 12.02 at the
 * 3840x2160 output, sets of 1920x1080 and 3840x2160 registered and filled the
 * screen; 1440x1080, 1440x960 and 1280x720 were refused with
 * 0x80290005 ("Buffer Resolution Error"). A swapchain of one of the taken
 * sizes is its framebuffers. Any other size renders into images of its own,
 * and each present blits the image into a framebuffer of the smallest taken
 * size that holds it: scaled to fit, keeping its aspect ratio, centred, with
 * black bars where the shapes differ.
 */

#ifndef WSI_COMMON_VIDEOOUT_SCALE_H
#define WSI_COMMON_VIDEOOUT_SCALE_H

#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan_core.h>

/* The framebuffer sizes VideoOut was measured taking, smallest first. */
static const VkExtent2D wsi_videoout_taken_sizes[] = {
   {1920, 1080},
   {3840, 2160},
};

static inline bool
wsi_videoout_extent_equal(VkExtent2D a, VkExtent2D b)
{
   return a.width == b.width && a.height == b.height;
}

/* Whether VideoOut takes framebuffers of this size. */
static inline bool
wsi_videoout_size_taken(VkExtent2D extent)
{
   for (unsigned i = 0; i < sizeof(wsi_videoout_taken_sizes) / sizeof(wsi_videoout_taken_sizes[0]); i++)
      if (wsi_videoout_extent_equal(extent, wsi_videoout_taken_sizes[i]))
         return true;
   return false;
}

/* The framebuffer size a swapchain of this size presents into: its own when
 * VideoOut takes it, else the smallest taken size that holds it, else the
 * largest. */
static inline VkExtent2D
wsi_videoout_target_extent(VkExtent2D extent)
{
   const unsigned count = sizeof(wsi_videoout_taken_sizes) / sizeof(wsi_videoout_taken_sizes[0]);
   if (wsi_videoout_size_taken(extent))
      return extent;
   for (unsigned i = 0; i < count; i++)
      if (extent.width <= wsi_videoout_taken_sizes[i].width && extent.height <= wsi_videoout_taken_sizes[i].height)
         return wsi_videoout_taken_sizes[i];
   return wsi_videoout_taken_sizes[count - 1];
}

/* The part of a target framebuffer an image of the given size fills: as large
 * as fits with its aspect ratio kept (rounded to the nearest pixel, at least
 * one), centred. */
static inline VkRect2D
wsi_videoout_fit_rect(VkExtent2D image, VkExtent2D target)
{
   uint32_t width, height;
   if (image.width == 0 || image.height == 0)
      return (VkRect2D){.offset = {0, 0}, .extent = target};
   if ((uint64_t)image.width * target.height <= (uint64_t)image.height * target.width) {
      /* As tall as the target or taller in shape: full height. */
      height = target.height;
      width = (uint32_t)(((uint64_t)image.width * target.height * 2 + image.height) / (image.height * UINT64_C(2)));
   } else {
      width = target.width;
      height = (uint32_t)(((uint64_t)image.height * target.width * 2 + image.width) / (image.width * UINT64_C(2)));
   }
   if (width == 0)
      width = 1;
   if (height == 0)
      height = 1;
   if (width > target.width)
      width = target.width;
   if (height > target.height)
      height = target.height;
   return (VkRect2D){
      .offset = {(int32_t)((target.width - width) / 2), (int32_t)((target.height - height) / 2)},
      .extent = {width, height},
   };
}

/* Whether the rectangle covers the whole target, leaving no bars to clear. */
static inline bool
wsi_videoout_rect_covers(VkRect2D rect, VkExtent2D target)
{
   return rect.offset.x == 0 && rect.offset.y == 0 && wsi_videoout_extent_equal(rect.extent, target);
}

#endif /* WSI_COMMON_VIDEOOUT_SCALE_H */
