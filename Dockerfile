FROM debian:bookworm

# install debian packages:
ENV DEBIAN_FRONTEND=noninteractive
RUN set -e -x; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
        locales \
        cmake pkg-config gkrellm libgtk2.0-dev txt2tags gcc \
        devscripts fakeroot debhelper build-essential \
        clang-format \
        gosu sudo

# setup su and locale
RUN set -e -x; \
    sed -i '/pam_rootok.so$/aauth sufficient pam_permit.so' /etc/pam.d/su; \
    echo 'en_US.UTF-8 UTF-8' >> /etc/locale.gen; locale-gen
ENV LC_ALL=en_US.UTF-8

CMD set -e -x; \
    cmake .; \
    make

