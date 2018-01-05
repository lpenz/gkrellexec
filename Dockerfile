FROM debian:stretch

# install debian packages:
ENV DEBIAN_FRONTEND=noninteractive
RUN set -x -e; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
        locales \
        cmake pkg-config gkrellm libgtk2.0-dev txt2tags gcc \
        devscripts fakeroot debhelper build-essential \
        clang-format \
        gosu sudo

# setup sudo and locale
RUN set -x -e; \
    echo 'ALL ALL=NOPASSWD:ALL' > /etc/sudoers.d/all; \
    chmod 0400 /etc/sudoers.d/all; \
    echo 'en_US.UTF-8 UTF-8' >> /etc/locale.gen; \
    locale-gen
ENV LC_ALL=en_US.UTF-8

# setup entrypoint with user UID/GID from host
RUN set -x -e; \
    (\
    echo '#!/bin/bash'; \
    echo 'MY_UID=${MY_UID:-1000}'; \
    echo 'set -x -e'; \
    echo 'useradd -M -u "$MY_UID" -o user'; \
    echo 'chown user:user /home/user'; \
    echo 'cd /home/user/work'; \
    echo 'exec gosu user "${@:-/bin/bash}"'; \
    ) > /usr/local/bin/entrypoint.sh; \
    chmod a+x /usr/local/bin/entrypoint.sh
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]

# If your UID is 1000, you can simply run the container as
# docker run -it --rm -v $PWD:/home/user/work ansible-playbooks
# otherwise, run it as:
# docker run -it --rm -v $PWD:/home/user/work -e MY_UID=$UID ansible-playbooks
