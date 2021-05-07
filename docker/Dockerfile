# Archlinux has the freshest packages
FROM archlinux:latest

# Intention is to mount the source directory here with the -v flag
VOLUME /omulator
WORKDIR /omulator

# Minimal set of deps for gcc and clang toolchains
RUN pacman -Syyu --noconfirm gcc clang lld cmake sdl2 ruby python ninja && ln -s /usr/share/clang/run-clang-tidy.py /usr/bin/run-clang-tidy

ENTRYPOINT ["./build.rb"]
CMD ["--help"]
