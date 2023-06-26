project(mini-ddns C)
add_executable(ifwatch ifwatch.c)
add_executable(monitor monitor.c)
target_compile_definitions(monitor PRIVATE
	-DINTERFACE_NAME="ppp3"
	-DCOMMAND="./newip.sh"
)