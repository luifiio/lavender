find_path(TAGLIB_INCLUDE_DIR taglib/fileref.h
    HINTS /opt/homebrew/Cellar/taglib
)

find_library(TAGLIB_LIBRARY NAMES tag
    HINTS /opt/homebrew/Cellar/taglib
)

if (TAGLIB_INCLUDE_DIR AND TAGLIB_LIBRARY)
    set(TAGLIB_FOUND TRUE)
else ()
    set(TAGLIB_FOUND FALSE)
endif ()
