#!/bin/bash
# 清理安装缓存（普通用户 / root 均可运行）
#
# 分组：常规清理（开发缓存 + 用户缓存 + 日志 + 临时文件）
#       包管理器缓存（apt/yum/dnf/pacman/zypper/pip）
#       应用与容器缓存（Snap/Flatpak/Docker）、崩溃转储
#
# 安全约定：~/.cache 只删 30 天未访问的文件，回收站只清 30 天前的条目，find 带 -xdev 不跨挂载点。

readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m'

IS_ROOT=0
if [[ $EUID -eq 0 ]]; then
    IS_ROOT=1
fi

# 清理门槛（天）
CACHE_AGE_DAYS=30
TRASH_AGE_DAYS=30
TMP_AGE_DAYS=7

# ---------- 通用工具 ----------
human() {
    numfmt --to=iec "$1" 2>/dev/null || echo "$1 字节"
}

# 目录大小（字节），路径不存在或已删除时返回 0
get_dir_size() {
    local size
    size=$(du -sb "$1" 2>/dev/null | cut -f1)
    echo "${size:-0}"
}

print_size_change() {
    local before=$1 after=$2
    local diff=$((before - after))
    if (( diff > 0 )); then
        echo -e "  ${GREEN}释放空间: $(human "$diff")${NC}"
    else
        echo -e "  ${YELLOW}未释放空间${NC}"
    fi
}

report() {
    local label="$1" before="$2" after="$3"
    echo "  $label: $(human "$before") -> $(human "$after")"
    print_size_change "$before" "$after"
}

show_disk_usage() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${GREEN}当前磁盘使用情况（根分区）：${NC}"
    df -h / | tail -1 2>/dev/null || echo "无法获取根分区信息"
    echo -e "${BLUE}========================================${NC}\n"
}

# purge_path <路径> <名称> [whole|contents|atime:N|mtime:N]
#   whole=整删目录  contents=清空内容保留目录  atime:N/mtime:N=按天数删文件
purge_path() {
    local path="$1" label="$2" mode="${3:-whole}" days
    [[ -e "$path" ]] || return 0
    local before after
    before=$(get_dir_size "$path")
    case "$mode" in
        whole)
            rm -rf "$path"
            ;;
        contents)
            find "$path" -xdev -mindepth 1 -delete 2>/dev/null
            ;;
        atime:*)
            days="${mode#atime:}"
            find "$path" -xdev -mindepth 1 -type f -atime "+$days" -delete 2>/dev/null
            find "$path" -xdev -mindepth 1 -type d -empty -delete 2>/dev/null
            ;;
        mtime:*)
            days="${mode#mtime:}"
            find "$path" -xdev -mindepth 1 -type f -mtime "+$days" -delete 2>/dev/null
            find "$path" -xdev -mindepth 1 -type d -empty -delete 2>/dev/null
            ;;
    esac
    after=$(get_dir_size "$path")
    report "$label" "$before" "$after"
}

# 待处理的用户家目录：普通用户只有自己，root 加上 /home/*
target_homes() {
    echo "$HOME"
    [[ $IS_ROOT -eq 1 ]] || return 0
    local home
    for home in /home/*; do
        [[ -d "$home" ]] && echo "$home"
    done
}

# 包管理器缓存目录的通用清理
generic_clean() {
    local clean_cmd="$1" cache_dir="$2" name="$3"
    echo -e "${YELLOW}清理 $name 缓存...${NC}"
    local before after
    before=$(get_dir_size "$cache_dir")
    eval "$clean_cmd"
    after=$(get_dir_size "$cache_dir")
    report "$name" "$before" "$after"
    echo ""
}

# ---------- 一、常规清理项（普通用户与 root 通用） ----------

# uv / go / yarn / pnpm / cargo / npm / conda
clean_dev_caches() {
    echo -e "${YELLOW}清理开发工具缓存（uv/conda/npm/go/cargo/pre-commit）...${NC}"
    local home tag
    while read -r home; do
        tag="$(basename "$home")"
        purge_path "$home/.cache/uv" "uv($tag)" whole
        purge_path "$home/.cache/pre-commit" "pre-commit($tag)" whole
        purge_path "$home/.cache/go-build" "go 构建缓存($tag)" whole
        purge_path "$home/.cache/yarn" "yarn($tag)" whole
        purge_path "$home/.cache/pnpm" "pnpm($tag)" whole
        purge_path "$home/.cargo/registry/cache" "cargo 包缓存($tag)" contents
        purge_path "$home/.npm/_cacache" "npm($tag)" contents
    done < <(target_homes)

    if command -v conda &>/dev/null; then
        echo -e "${YELLOW}  执行 conda clean -a -y${NC}"
        if conda clean -a -y >/dev/null 2>&1; then
            echo -e "${GREEN}  conda 缓存清理完成${NC}"
        else
            echo -e "${YELLOW}  conda 清理失败或被跳过${NC}"
        fi
    fi
    echo -e "${GREEN}开发工具缓存清理完成${NC}\n"
}

# 当前用户的 ~/.cache（仅过期文件）、缩略图、回收站
clean_user_cache() {
    echo -e "${YELLOW}清理用户缓存目录、缩略图与回收站...${NC}"
    purge_path "$HOME/.cache" "~/.cache 中 ${CACHE_AGE_DAYS} 天未访问的文件" "atime:$CACHE_AGE_DAYS"
    purge_path "$HOME/.cache/thumbnails" "缩略图缓存" contents
    purge_path "$HOME/.local/share/Trash" "回收站中 ${TRASH_AGE_DAYS} 天前的文件" "mtime:$TRASH_AGE_DAYS"
    echo -e "${GREEN}用户缓存清理完成${NC}\n"
}

# 系统日志（需要 root）
clean_logs() {
    if [[ $IS_ROOT -eq 0 ]]; then
        echo -e "${YELLOW}跳过系统日志清理（需要 root 权限）${NC}\n"
        return
    fi
    echo -e "${YELLOW}清理系统日志...${NC}"
    echo "  Journal 清理前: $(journalctl --disk-usage 2>/dev/null | tail -1)"
    journalctl --vacuum-time=${TMP_AGE_DAYS}d >/dev/null 2>&1
    echo "  Journal 清理后: $(journalctl --disk-usage 2>/dev/null | tail -1)"
    # 只删轮转归档，保留正在使用的日志
    find /var/log -xdev -type f \( -name "*.gz" -o -name "*.1" -o -name "*.log.[0-9]" \) \
        -mtime +30 -delete 2>/dev/null
    echo "  已删除 30 天前的轮转归档日志"
    echo -e "${GREEN}系统日志清理完成${NC}\n"
}

# 系统临时文件（需要 root）
clean_tmp() {
    if [[ $IS_ROOT -eq 0 ]]; then
        echo -e "${YELLOW}跳过系统临时文件清理（需要 root 权限）${NC}\n"
        return
    fi
    echo -e "${YELLOW}清理系统临时文件...${NC}"
    purge_path "/tmp" "/tmp 中 ${TMP_AGE_DAYS} 天未访问的文件" "atime:$TMP_AGE_DAYS"
    echo -e "${GREEN}系统临时文件清理完成${NC}\n"
}

# 崩溃转储（需要 root）
clean_coredump() {
    if [[ $IS_ROOT -eq 0 ]]; then
        echo -e "${YELLOW}跳过崩溃转储清理（需要 root 权限）${NC}\n"
        return
    fi
    echo -e "${YELLOW}清理崩溃转储...${NC}"
    purge_path "/var/lib/systemd/coredump" "systemd 崩溃转储" contents
    purge_path "/var/crash" "崩溃报告" contents
    echo -e "${GREEN}崩溃转储清理完成${NC}\n"
}

# ---------- 二、包管理器缓存（pip 无需 root） ----------

# pip 缓存：普通用户清自己，root 清所有用户
clean_pip() {
    echo -e "${YELLOW}清理 pip 缓存...${NC}"
    local home cache hit=0
    while read -r home; do
        cache="$home/.cache/pip"
        [[ -d "$cache" ]] || continue
        hit=1
        purge_path "$cache" "pip($(basename "$home"))" whole
    done < <(target_homes)
    (( hit == 1 )) || echo -e "${YELLOW}未找到 pip 缓存目录${NC}"
    echo -e "${GREEN}pip 缓存清理完成${NC}\n"
}

clean_apt() {
    if [[ $IS_ROOT -eq 1 ]]; then
        if command -v apt &>/dev/null; then
            generic_clean "apt-get autoclean -y && apt-get autoremove -y && apt-get clean -y" \
                "/var/cache/apt/archives" "APT"
        else
            echo -e "${YELLOW}APT 未安装，跳过${NC}\n"
        fi
    else
        echo -e "${YELLOW}跳过 APT 缓存清理（需要 root 权限）${NC}\n"
    fi
}

clean_yum() {
    if [[ $IS_ROOT -eq 1 ]]; then
        if command -v yum &>/dev/null; then
            generic_clean "yum clean all && yum autoremove -y" \
                "/var/cache/yum" "YUM"
        else
            echo -e "${YELLOW}YUM 未安装，跳过${NC}\n"
        fi
    else
        echo -e "${YELLOW}跳过 YUM 缓存清理（需要 root 权限）${NC}\n"
    fi
}

clean_dnf() {
    if [[ $IS_ROOT -eq 1 ]]; then
        if command -v dnf &>/dev/null; then
            generic_clean "dnf clean all && dnf autoremove -y" \
                "/var/cache/dnf" "DNF"
        else
            echo -e "${YELLOW}DNF 未安装，跳过${NC}\n"
        fi
    else
        echo -e "${YELLOW}跳过 DNF 缓存清理（需要 root 权限）${NC}\n"
    fi
}

clean_pacman() {
    if [[ $IS_ROOT -eq 1 ]]; then
        if command -v pacman &>/dev/null; then
            echo -e "${YELLOW}清理 Pacman 缓存...${NC}"
            local cache_dir="/var/cache/pacman/pkg" before after
            before=$(get_dir_size "$cache_dir")
            if command -v paccache &>/dev/null; then
                paccache -r
                paccache -rk2
            else
                # 降级：保留最近 2 个版本
                find "$cache_dir" -xdev -type f -name "*.pkg.tar.*" -printf '%T@ %p\n' | \
                    sort -rn | awk 'NR>2 {print $2}' | xargs -r rm -f
            fi
            after=$(get_dir_size "$cache_dir")
            report "Pacman" "$before" "$after"
            echo ""
        else
            echo -e "${YELLOW}Pacman 未安装，跳过${NC}\n"
        fi
    else
        echo -e "${YELLOW}跳过 Pacman 缓存清理（需要 root 权限）${NC}\n"
    fi
}

clean_zypper() {
    if [[ $IS_ROOT -eq 1 ]]; then
        if command -v zypper &>/dev/null; then
            echo -e "${YELLOW}清理 Zypper 缓存...${NC}"
            local cache_dir="/var/cache/zypp/packages" before after
            before=$(get_dir_size "$cache_dir")
            zypper clean --all
            LANG=C zypper packages --orphaned | awk 'NR>2 {print $5}' | xargs -r zypper remove -y
            after=$(get_dir_size "$cache_dir")
            report "Zypper" "$before" "$after"
            echo ""
        else
            echo -e "${YELLOW}Zypper 未安装，跳过${NC}\n"
        fi
    else
        echo -e "${YELLOW}跳过 Zypper 缓存清理（需要 root 权限）${NC}\n"
    fi
}

# ---------- 三、应用与容器缓存（需要 root） ----------
clean_snap() {
    if [[ $IS_ROOT -eq 0 ]]; then
        echo -e "${YELLOW}跳过 Snap 缓存清理（需要 root 权限）${NC}\n"
        return
    fi
    if ! command -v snap &>/dev/null; then
        echo -e "${YELLOW}Snap 未安装，跳过${NC}\n"
        return
    fi
    echo -e "${YELLOW}清理 Snap 缓存...${NC}"
    local cache_dir="/var/lib/snapd/cache" before after
    before=$(get_dir_size "$cache_dir")
    LANG=C snap list --all | awk '/disabled/{print $1, $3}' | while read -r snapname revision; do
        snap remove "$snapname" --revision="$revision"
    done
    rm -rf "${cache_dir:?}/"* 2>/dev/null
    after=$(get_dir_size "$cache_dir")
    report "Snap 下载缓存" "$before" "$after"
    echo -e "${GREEN}Snap 缓存清理完成${NC}\n"
}

clean_flatpak() {
    if [[ $IS_ROOT -eq 0 ]]; then
        echo -e "${YELLOW}跳过 Flatpak 缓存清理（需要 root 权限）${NC}\n"
        return
    fi
    if ! command -v flatpak &>/dev/null; then
        echo -e "${YELLOW}Flatpak 未安装，跳过${NC}\n"
        return
    fi
    echo -e "${YELLOW}清理 Flatpak 缓存...${NC}"
    flatpak uninstall --unused -y
    flatpak repair
    echo -e "${GREEN}Flatpak 缓存清理完成${NC}\n"
}

clean_docker() {
    if [[ $IS_ROOT -eq 0 ]]; then
        echo -e "${YELLOW}跳过 Docker 资源清理（需要 root 权限）${NC}\n"
        return
    fi
    if ! command -v docker &>/dev/null; then
        echo -e "${YELLOW}Docker 未安装，跳过${NC}\n"
        return
    fi
    echo -e "${YELLOW}清理 Docker 资源...${NC}"
    docker system prune -a -f
    docker builder prune -f >/dev/null 2>&1
    echo -e "${GREEN}Docker 资源清理完成${NC}\n"
}

# ---------- 四、清理组合与菜单 ----------
run_routine_clean() {
    echo -e "${BLUE}>>> 常规清理（开发工具缓存 + 用户缓存 + 日志/临时文件）${NC}\n"
    clean_dev_caches
    clean_user_cache
    clean_logs
    clean_tmp
}

run_extra_clean() {
    echo -e "${BLUE}>>> 应用与容器缓存清理${NC}\n"
    clean_snap
    clean_flatpak
    clean_docker
}

run_package_clean() {
    echo -e "${BLUE}>>> 包管理器缓存清理${NC}\n"
    clean_pip                                  # 无需 root
    [[ $IS_ROOT -eq 1 ]] || return 0
    clean_apt
    clean_yum
    clean_dnf
    clean_pacman
    clean_zypper
}

# 自定义表项 "名称|关键词|函数名"；组内成员不单列（如 pip 归在包管理器缓存组，输入关键词即可选中）
CUSTOM_ITEMS=()
CUSTOM_DONE=""

# 执行表项，同一次选择内去重
run_custom_item() {
    local name="$1" func="$2"
    if [[ "$CUSTOM_DONE" == *" $func "* ]]; then
        echo -e "${YELLOW}已处理过「$name」，跳过${NC}"
        return
    fi
    CUSTOM_DONE="$CUSTOM_DONE$func "
    "$func"
}

# 数字按编号，其它按名称/关键词匹配
resolve_custom() {
    local token="$1" needle i item name keys func hit=0
    if [[ "$token" =~ ^[0-9]+$ ]]; then
        if (( token >= 1 && token <= ${#CUSTOM_ITEMS[@]} )); then
            item="${CUSTOM_ITEMS[$((token - 1))]}"
            run_custom_item "${item%%|*}" "${item##*|}"
        else
            echo -e "${RED}无效编号: $token${NC}"
        fi
        return
    fi
    needle="$(printf '%s' "$token" | tr '[:upper:]' '[:lower:]')"
    for i in "${!CUSTOM_ITEMS[@]}"; do
        item="${CUSTOM_ITEMS[$i]}"
        name="${item%%|*}"
        keys="${item#*|}"; keys="${keys%|*}"
        func="${item##*|}"
        if [[ "$(printf '%s' "$name" | tr '[:upper:]' '[:lower:]')" == *"$needle"* ]] \
            || [[ " $keys " == *" $needle "* ]]; then
            hit=1
            run_custom_item "$name" "$func"
        fi
    done
    (( hit == 1 )) || echo -e "${RED}无法识别的选择: $token${NC}"
}

custom_clean() {
    if [[ $IS_ROOT -eq 1 ]]; then
        CUSTOM_ITEMS=(
            "包管理器缓存|apt yum dnf pacman zypper pip|run_package_clean"
            "开发工具缓存|uv conda npm go cargo yarn pnpm pre-commit|clean_dev_caches"
            "用户缓存与回收站|用户 缓存 回收站 trash|clean_user_cache"
            "Snap 缓存|snap|clean_snap"
            "Flatpak 缓存|flatpak|clean_flatpak"
            "Docker 资源|docker 容器 镜像|clean_docker"
            "系统日志|日志 log journal|clean_logs"
            "临时文件|临时 tmp temp|clean_tmp"
            "崩溃转储|崩溃 coredump core|clean_coredump"
        )
    else
        CUSTOM_ITEMS=(
            "包管理器缓存|pip|run_package_clean"
            "开发工具缓存|uv conda npm go cargo yarn pnpm pre-commit|clean_dev_caches"
            "用户缓存与回收站|用户 缓存 回收站 trash|clean_user_cache"
        )
    fi

    CUSTOM_DONE=" "
    local i item token
    echo -e "\n${YELLOW}请选择要清理的项目：${NC}"
    for i in "${!CUSTOM_ITEMS[@]}"; do
        item="${CUSTOM_ITEMS[$i]}"
        printf '  %2d. %s\n' $((i + 1)) "${item%%|*}"
    done
    echo -e "${BLUE}可输入编号（多个用空格分隔），也可直接输入名称或关键词，例如：${NC}"
    echo -e "${BLUE}  包管理器 / 开发工具 / 用户 / snap / docker${NC}"
    echo -n "请输入选择: "
    read -r line
    for token in $line; do
        resolve_custom "$token"
    done
}

# ---------- 主函数 ----------
main() {
    clear
    echo -e "${BLUE}========================================${NC}"
    if [[ $IS_ROOT -eq 1 ]]; then
        echo -e "${GREEN}       Linux 系统安装缓存清理工具 (root 模式)       ${NC}"
    else
        echo -e "${GREEN}       Linux 用户缓存清理工具 (普通用户模式)       ${NC}"
    fi
    echo -e "${BLUE}========================================${NC}\n"

    show_disk_usage

    if [[ $IS_ROOT -eq 1 ]]; then
        echo "请选择要清理的项目："
        echo "1. 常规清理（推荐）：开发工具缓存 + 用户缓存 + 日志/临时文件"
        echo "2. 全部清理：常规清理 + 包管理器缓存 + Snap/Flatpak/Docker + 崩溃转储"
        echo "3. 仅清理包管理器缓存（APT/YUM/DNF/Pacman/Zypper/pip）"
        echo "4. 仅清理应用与容器缓存（Snap/Flatpak/Docker）"
        echo "5. 仅清理日志、临时文件与崩溃转储"
        echo "6. 自定义选择"
        echo -n "请输入选择 [1-6]: "
        read -r choice

        case $choice in
            1)
                run_routine_clean
                ;;
            2)
                run_routine_clean
                run_package_clean
                run_extra_clean
                clean_coredump
                ;;
            3)
                run_package_clean
                ;;
            4)
                run_extra_clean
                ;;
            5)
                clean_logs
                clean_tmp
                clean_coredump
                ;;
            6)
                custom_clean
                ;;
            *)
                echo -e "${RED}无效选择，退出脚本${NC}"
                exit 1
                ;;
        esac
    else
        # 普通用户菜单（只清自己家目录）
        echo "请选择要清理的项目："
        echo "1. 常规清理（推荐）：开发工具缓存 + 用户缓存与回收站"
        echo "2. 仅清理包管理器缓存"
        echo "3. 仅清理开发工具缓存（uv/conda/npm/go/cargo/pre-commit）"
        echo "4. 仅清理用户缓存与回收站"
        echo "5. 自定义选择"
        echo -n "请输入选择 [1-5]: "
        read -r choice

        case $choice in
            1)
                run_routine_clean
                ;;
            2)
                run_package_clean
                ;;
            3)
                clean_dev_caches
                ;;
            4)
                clean_user_cache
                ;;
            5)
                custom_clean
                ;;
            *)
                echo -e "${RED}无效选择，退出脚本${NC}"
                exit 1
                ;;
        esac
    fi

    echo -e "${BLUE}========================================${NC}"
    echo -e "${GREEN}清理完成！${NC}"
    show_disk_usage

    # 可选：重启日志服务
    if [[ $IS_ROOT -eq 1 ]]; then
        read -p "是否重启 systemd-journald 日志服务？(y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            systemctl restart systemd-journald
            echo -e "${GREEN}日志服务已重启${NC}"
        fi
    fi
}

main "$@"
