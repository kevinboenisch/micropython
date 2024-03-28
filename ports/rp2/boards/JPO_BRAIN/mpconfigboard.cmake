# cmake file for Raspberry Pi Pico

### Prerequisites
if (NOT DEFINED ENV{JPO_PATH})
	message(FATAL_ERROR "Environment variable JPO_PATH not set.")
endif()
# Fix backslashes in Windows
file(TO_CMAKE_PATH "$ENV{JPO_PATH}" JPO_PATH)
message("JPO_PATH (from environment) is ${JPO_PATH}")

# Set board
set(JPO_BOARD_DIR "${JPO_PATH}/resources/build_config")
if (NOT EXISTS "${JPO_BOARD_DIR}")
	message(FATAL_ERROR "Directory '${JPO_BOARD_DIR}' does not exist.")
endif()
set(PICO_BOARD_HEADER_DIRS "${JPO_BOARD_DIR}")
set(PICO_BOARD "jpo_brain")
