cmake_minimum_required(VERSION 3.8)
project(coral_multiplexor)

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rosidl_default_generators REQUIRED)

ament_python_install_package(coral_multiplexor)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/CommandItem.msg"
  "msg/DebugEvent.msg"
  "msg/ExchangeEvent.msg"
  "msg/ReactionEvent.msg"
  "msg/TicketState.msg"
  "msg/Value.msg"
  "srv/CloseTicket.srv"
  "srv/GetBridgeStatus.srv"
  "srv/GetHealth.srv"
  "srv/GetTicketInfo.srv"
  "srv/RegisterClient.srv"
  "srv/ReloadBridge.srv"
  "srv/SubmitTicket.srv"
  "srv/UnregisterClient.srv"
)

install(PROGRAMS
  scripts/coral_multiplexor_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_export_dependencies(rosidl_default_runtime)
ament_package()
