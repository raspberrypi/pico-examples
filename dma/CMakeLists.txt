if (TARGET hardware_dma)
    add_subdirectory_exclude_platforms(channel_irq)
    add_subdirectory_exclude_platforms(control_blocks)
    add_subdirectory_exclude_platforms(hello_dma)
    add_subdirectory_exclude_platforms(sniff_crc)
else()
    message("Skipping DMA examples as hardware_dma is unavailable on this platform")
endif()

