FROM ubuntu:jammy

RUN apt update && apt upgrade -y && apt install -y wget

# Per https://vulkan.lunarg.com/doc/view/latest/linux/getting_started_ubuntu.html#user-content-ubuntu-2204-jammy-jellyfish
# N.B. this is tied to jammy
RUN wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | tee /etc/apt/trusted.gpg.d/lunarg.asc && wget -qO /etc/apt/sources.list.d/lunarg-vulkan-jammy.list http://packages.lunarg.com/vulkan/lunarg-vulkan-jammy.list
RUN apt update && apt install -y build-essential git cmake ninja-build ruby clang lld libsdl2-dev libreadline-dev vulkan-sdk
