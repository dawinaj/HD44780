
idf_component_register(	SRCS "main.cpp" "src/hbridge.cpp"
						INCLUDE_DIRS "." "include/" 
						REQUIRES driver HD44780)

component_compile_options(-std=gnu++2a)
