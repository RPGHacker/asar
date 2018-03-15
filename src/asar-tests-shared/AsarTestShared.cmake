
# Shared settings for Asar test applications

macro(set_asar_test_shared_properties target)
	# Enable maximum warning level
	
	if(MSVC)
		target_compile_definitions(${target} PRIVATE "_CRT_SECURE_NO_WARNINGS")
			
		if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
			string(REGEX REPLACE "/W[0-4]" "/Wall" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
		else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall")
		endif()		
		
		# These certainly aren't worth a warning, though
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4514")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4710")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4820")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")
		target_link_libraries(${target} dl)
	endif()
endmacro()
