FROM swr.cn-north-4.myhuaweicloud.com/cloud_boostkit/openeuler-22.03-lts-sp4:latest

# ── 第一层: 系统工具链 + 构建依赖 (dnf) ──────────────────────────
RUN dnf install -y \
      gcc gcc-c++ make cmake git python3-pip \
      boost-devel sqlite sqlite-devel \
      curl pkgconfig wget unzip \
    && dnf clean all \
    && rm -rf /var/cache/dnf

# ── 第二层: 源码编译 ragel 6.10 ──────────────────────────────────
# ragel 是 hyperscan 编译期必需的状态机编译器, dnf 仓库无此包, 必须源码编译
RUN cd /tmp \
    && curl -sL -o ragel-6.10.tar.gz https://buildtools.obs.cn-north-4.myhuaweicloud.com/ragel-6.10.tar.gz \
    && tar -xzf ragel-6.10.tar.gz \
    && cd ragel-6.10 \
    && ./configure --prefix=/usr/local \
    && make -j$(nproc) \
    && make install \
    && cd /tmp && rm -rf ragel-6.10 ragel-6.10.tar.gz

# ── 第三层: 预下载 pcre 8.45 源码到 /opt ────────────────────────
# pcre.cmake 从仓库 pcre/ 子目录构建, 不能用系统 pcre-devel 替代
# 容器启动后需 cp -rf /opt/pcre-8.45 /workspace/pcre
RUN cd /tmp \
    && curl -sL -o pcre-8.45.zip https://buildtools.obs.cn-north-4.myhuaweicloud.com/pcre-8.45.zip \
    && unzip pcre-8.45.zip \
    && mv pcre-8.45 /opt/pcre-8.45 \
    && rm -f pcre-8.45.zip

# ── 第四层: 安装 boost 1.87.0 工具 ──────────────────────────────
# 下载 boost 1.87.0 源码到 /hyperscan, 并建立 include/boost 软链接供编译引用
RUN mkdir -p /hyperscan/include \
    && cd /hyperscan \
    && wget --no-check-certificate https://buildtools.obs.cn-north-4.myhuaweicloud.com/boost_1_87_0.tar.gz \
    && tar -zxf boost_1_87_0.tar.gz \
    && ln -s /hyperscan/boost_1_87_0/boost include/boost \
    && rm -f boost_1_87_0.tar.gz

# ── 第五层: 启动脚本 ────────────────────────────────────────────
# 自动拷贝 pcre 源码到 /workspace/pcre, 然后执行传入命令
RUN printf '#!/bin/bash\n\
set -e\n\
if [ ! -d /workspace/pcre ] && [ -d /opt/pcre-8.45 ]; then\n\
  cp -rf /opt/pcre-8.45 /workspace/pcre\n\
  echo "[entrypoint] pcre-8.45 源码已拷贝到 /workspace/pcre"\n\
fi\n\
exec "$@"\n' > /usr/local/bin/docker-entrypoint.sh \
    && chmod +x /usr/local/bin/docker-entrypoint.sh

# ── 验证依赖完整性 ───────────────────────────────────────────────
RUN echo "=== 依赖验证 ===" \
    && gcc --version | head -1 \
    && cmake --version | head -1 \
    && make --version | head -1 \
    && ragel -v | head -1 \
    && ls /usr/include/boost/version.hpp \
    && ls /hyperscan/boost_1_87_0/boost/version.hpp \
    && ls /opt/pcre-8.45/pcre.h.generic \
    && pkg-config --libs sqlite3 \
    && echo "=== 全部依赖就绪 ==="

WORKDIR /workspace

ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]
CMD ["sleep", "infinity"]
