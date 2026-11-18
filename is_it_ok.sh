#!/usr/bin/env bash
# IFJ26 submission archive checker. Original checker: Zbynek Krivka.
# Version 2.0 (2026-10-08)
set -u
export LC_ALL=C
MAX_ARCHIVE_SIZE=1500000

usage() {
    cat <<'HELP'
Použití: is_it_ok.sh [--keep-tmp-dir] ARCHIV.zip
Ověří formát archivu pro projekt z IFJ26. V podporovaném prostředí vyzkouší také
překlad a spuštění na triviálním kódu.

Archiv musí být ve formátu ZIP a být menší než 1 500 000 B.
  -h, --help       Zobrazí tuto nápovědu.
  --keep-tmp-dir   Neodstraní dočasný adresář, do kterého je archiv rozbalen.

Pro plnou funkcionalitu musí být skript spuštěn na stroji, kde je k dispozici
Docker či Podman; nebo na školním serveru merlin.fit.vutbr.cz.
HELP
}
keep_tmp=0
archive=
while (($#)); do
    case $1 in
        -h|--help) usage; exit 0 ;;
        --keep-tmp-dir) keep_tmp=1 ;;
        --) shift; if (($# == 1)) && [[ -z $archive ]]; then archive=$1; shift; break; else usage >&2; exit 2; fi ;;
        -*) printf 'Neznámý argument: %s\n' "$1" >&2; exit 2 ;;
        *) if [[ -n $archive ]]; then usage >&2; exit 2; fi; archive=$1 ;;
    esac
    shift
done
if [[ -z $archive ]]; then usage >&2; exit 2; fi
if [[ $archive != *.zip || ! -f $archive ]]; then
    printf 'ERROR: %s neexistuje nebo nemá příponu .zip\n' "$archive" >&2
    exit 2
fi
for tool in unzip zipinfo file find; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        printf 'ERROR: v systému není k dispozici požadovaný nástroj: %s\n' "$tool" >&2; exit 1
    fi
done
archive=$(cd -- "$(dirname -- "$archive")" && printf '%s/%s' "$PWD" "$(basename -- "$archive")") || exit 1
tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/ifj26-check.XXXXXXXX") || exit 1
image_tag=
runtime=
cleanup() {
    local status=$?
    trap - EXIT
    if [[ -n $image_tag ]]; then "$runtime" image rm "$image_tag" >> "$log" 2>&1 || true; fi
    if ((keep_tmp)); then
        printf 'Extrahuji do dočasného adresáře: %s\n' "$tmp_dir"
    else
        rm -rf -- "$tmp_dir"
    fi
    exit "$status"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
log="$PWD/ifj26_is-it-ok_$(date +%m-%d_%H-%M).log"
project=$tmp_dir/project
mkdir "$project"
: > "$log" || exit 1
errors=0
error() { printf 'ERROR: %s\n' "$*" | tee -a "$log"; errors=$((errors + 1)); }
ok() { printf 'OK: %s\n' "$*" | tee -a "$log"; }
warning() { printf 'WARNING: %s\n' "$*" | tee -a "$log"; }
archive_size=$(wc -c < "$archive")
if ((archive_size >= MAX_ARCHIVE_SIZE)); then
    error "Archiv má $archive_size B. Měl by mít maximálně $MAX_ARCHIVE_SIZE B."
else
    ok "Velikost archivu: $archive_size B"
fi

# Validate member paths before extraction; do not flatten or overwrite collisions.
if ! unzip -Z1 "$archive" > "$tmp_dir/members" 2>> "$log"; then
    error 'Chyba čtení rozbaleného archivu'; exit 1
fi
if [[ ! -s $tmp_dir/members ]]; then error 'ZIP archiv je prázdný'; exit 1; fi
if zipinfo -l "$archive" | grep -q '^l'; then error 'ZIP archiv nesmí obsahovat symlinky'; fi
declare -A seen=()
while IFS= read -r member || [[ -n $member ]]; do
    path=${member%/}
    if [[ ! $path =~ ^[A-Za-z0-9._-]+(/[A-Za-z0-9._-]+)*$ ]]; then
        error "V archivu je neplatná cesta: $member"; continue
    fi
    IFS=/ read -r -a components <<< "$path"
    for component in "${components[@]}"; do
        if [[ $component == .* || $component == __MACOSX ]]; then
            error "V archivu je zakázaný adresář: $member"
        fi
    done
    key=${path,,}
    if [[ -n ${seen[$key]+present} ]]; then error "V archivu je duplicitní jméno souboru (pozor na case sensitivity): $member"; fi
    seen[$key]=1
    case $path in
        src|tests) [[ $member == */ ]] || error "Neočekávaný adresář: $member" ;;
        tests/*) ;;
        src/*) [[ ${path#src/} != */* && $member != */ ]] || error "Zdrojový kód by neměl mít adresářovou strukturu: $member" ;;
        */*) error "Neočekávaný adresář: $member" ;;
        *) [[ $member != */ ]] || error "Neočekávaný adresář: $member" ;;
    esac
done < "$tmp_dir/members"
if ((errors)); then exit 1; fi
if ! unzip -q "$archive" -d "$project" >> "$log" 2>&1; then error 'Nelze rozbalit archiv'; exit 1; fi
ok 'Archiv lze rozbalit'

for required in Makefile dokumentace.pdf rozdeleni rozsireni; do
    if [[ -f $project/$required ]]; then ok "Nalezeno: $required"; else error "Nenalezen vyžadovaný soubor: $required"; fi
done
while IFS= read -r -d '' path; do
    relative=${path#"$project/"}
    case $relative in
        Makefile|dokumentace.pdf|rozdeleni|rozsireni|tests/*|*.c|*.h) ;;
        *) warning "V kořeni je podezřelý soubor (očekávány jen vyžadované soubory a soubory .c/.h): $relative" ;;
    esac
    description=$(file -b -- "$path")
    case $description in
        *ELF*|*Mach-O*|*PE32*|*'MS-DOS executable'*|*'current ar archive'*|*'compiled Java'*|*'byte-compiled'*)
            error "Nalezen nepovolený spustitelný binární soubor: ${path#"$project/"}" ;;
    esac
    case $path in
        *.o|*.obj|*.a|*.so|*.so.*|*.dll|*.exe|*.class|*.pyc|*.gch|*.pch|*.lib)
            error "Nalezen nepovolený binární soubor: ${path#"$project/"}" ;;
    esac
    if command -v dos2unix >/dev/null 2>&1; then
        dos2unix -- "$path" >> "$log" 2>&1 || error "Nelze použít dos2unix pro konverzi konců řádků: ${path#"$project/"}"
    fi
done < <(find "$project" -type f -print0)

check_metadata() {
    local path=$project/$1 line number=0 total=0 id login percent
    [[ -f $path ]] || return
    local -A ids=() logins=()
    while IFS= read -r line || [[ -n $line ]]; do
        number=$((number + 1))
        if [[ $1 == rozdeleni ]]; then
            if [[ ! $line =~ ^([0-9]{5,7}):(x([a-z]{5,6}[a-z0-9]{2}|[0-9]{5,7})):([0-9]{1,3})$ ]]; then
                error "Neplatný řádek v souboru 'rozdeleni': $number: $line"; continue
            fi
            id=${BASH_REMATCH[1]}; login=${BASH_REMATCH[2]}; percent=$((10#${BASH_REMATCH[4]}))
            if [[ -n ${ids[$id]+yes} || -n ${logins[$login]+yes} ]]; then error "Duplicitní login v 'rozdeleni' na řádku $number"; fi
            ids[$id]=1; logins[$login]=1
            if ((percent > 100)); then error "Součet procent v 'rozdeleni' je >100"; fi
            total=$((total + percent))
        else
            case $line in
                FUNEXP|BOOLTHEN|CONTROL|VALMEM|ALGEBDT|INTERPOLATEDS|LAMBDAS|LISTS|EXTKW|PARAMLISTS) ;;
                *) error "Na řádku $number souboru 'rozsireni' je neplatný identifikátor rozšíření: $line" ;;
            esac
        fi
    done < "$path"
    if [[ -s $path && $(tail -c 1 "$path" | od -An -tu1 | tr -d ' ') != 10 ]]; then
        error "$1: na konci souboru chybí LF"
    fi
    if [[ $1 == rozdeleni ]] && ((total != 100)); then error "Součet procent v 'rozdeleni' je $total (očekává se 100)"; fi
}
check_metadata rozdeleni
check_metadata rozsireni

cat > "$tmp_dir/example.ifj" <<'EXAMPLE'
import ifj26.*

@main def main(): Unit =
  print("Hello, IFJ26!\n")
EXAMPLE

# Keep this embedded Dockerfile identical to the project-root Dockerfile.
cat > "$tmp_dir/Dockerfile" <<'DOCKERFILE'
# syntax=docker/dockerfile:1
# Copy this file into the root of an extracted IFJ26 submission, then run:
#   docker build -t ifj26-project .
#   docker run --rm -i ifj26-project < program.ifj > program.ifjcode
# The common toolchain can also be built with --target build-environment.

ARG DEBIAN_IMAGE=debian:trixie-slim@sha256:a99cfc517144bc59b1978475ec53b46ecabec7e43635402ee5b77cc54cd1b20a

# Stage 1: shared environment; no submission files affect these layers.
FROM ${DEBIAN_IMAGE} AS build-environment

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        clang-19 \
        clang-format \
        clang-tidy \
        cmake \
        cppcheck \
        curl \
        git \
        libclang-rt-19-dev \
        libgmp10 \
        libmpc3 \
        libmpfr6 \
        libzstd1 \
        zlib1g \
    && rm -rf /var/lib/apt/lists/*

# LLVM provides Clang 23 packages built for Trixie. Scope trust to this repo.
RUN curl --fail --show-error --silent --location --retry 3 \
        https://apt.llvm.org/llvm-snapshot.gpg.key \
        --output /usr/share/keyrings/llvm.asc \
    && printf '%s\n' \
        'deb [signed-by=/usr/share/keyrings/llvm.asc] https://apt.llvm.org/trixie/ llvm-toolchain-trixie-23 main' \
        > /etc/apt/sources.list.d/llvm.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends clang-23 libclang-rt-23-dev \
    && rm -rf /var/lib/apt/lists/*

# Import the official Trixie GCC installations, keeping versions isolated.
# External COPY sources do not add build stages. The digests pin the images.
COPY --from=gcc:14.4.0-trixie@sha256:9188ac751ca24431dc43dbd142a223c98ea74f01d2858e84d30ba342a0d67844 /usr/local/ /opt/gcc-14.4/
COPY --from=gcc:15.3.0-trixie@sha256:ead103e6d03b69232962d467f3520c3f70b6718c69ff71efcc08efe9011fadb6 /usr/local/ /opt/gcc-15.3/
COPY --from=gcc:16.2.0-trixie@sha256:ef558a40d1f13115293feee01526dbdb9aaad7c9c5a00da05f471ce042e855c1 /usr/local/ /opt/gcc-16.2/

# Merlin paths observed on 2026-10-05. /usr/bin/gcc remains Debian GCC 14;
# PATH selects /usr/local/bin/gcc (15.3) and /usr/bin/clang (19).
RUN set -eu; \
    for version in 14.4 15.3 16.2; do \
        for tool in gcc g++ cpp gcc-ar gcc-nm gcc-ranlib gcov gcov-dump gcov-tool; do \
            ln -s "/opt/gcc-${version}/bin/${tool}" "/usr/local/bin/${tool}-${version}"; \
        done; \
        ln -s "gcc-${version}" "/usr/local/bin/x86_64-linux-gcc-${version}"; \
        ln -s "gcc-${version}" "/usr/local/bin/x86_64-linux-gcc-${version}.0"; \
    done; \
    for tool in gcc g++ cpp gcc-ar gcc-nm gcc-ranlib gcov gcov-dump gcov-tool; do \
        ln -s "${tool}-15.3" "/usr/local/bin/${tool}"; \
    done; \
    ln -s gcc /usr/local/bin/cc; \
    ln -s /usr/lib/llvm-19 /usr/local/share/Clang-19.0; \
    ln -s /usr/lib/llvm-23 /usr/local/share/Clang-23.0; \
    ln -s Clang-23.0 /usr/local/share/Clang; \
    ln -s ../lib/llvm-19/bin/clang /usr/bin/clang; \
    ln -s ../lib/llvm-19/bin/clang++ /usr/bin/clang++; \
    printf '%s\n' /opt/gcc-16.2/lib64 /opt/gcc-15.3/lib64 /opt/gcc-14.4/lib64 \
        > /etc/ld.so.conf.d/000-gcc.conf; \
    ldconfig; \
    install -d -o 1000 -g 1000 /build /out /out/lib

ENV PATH="/usr/local/bin:/usr/bin:/bin:/usr/local/share/Clang/bin"
WORKDIR /build

# Stage 2: build the submitted Makefile's default target as an ordinary user.
FROM build-environment AS build
COPY --chown=1000:1000 . .
USER 1000:1000

# The executable may have any name. Find ELF executables (including PIE),
# excluding optional tests; executable scripts and object files do not count.
# Leave make's compiler, flags, target and job count to the submitted Makefile.
# Use line continuations: the legacy builder may silently discard a heredoc.
# Collect shared libraries for dynamic executables; static ones need none.
RUN set -eu; \
    make; \
    find . -path ./tests -prune -o -type f -perm /111 -print > /out/candidates; \
    count=0; \
    binary=; \
    while IFS= read -r candidate; do \
        if readelf -h "$candidate" 2>/dev/null | grep -Eq 'Type:.*(EXEC|DYN)' \
            && readelf -h "$candidate" 2>/dev/null | grep -Eq 'Entry point address:.*0x[1-9a-fA-F]'; then \
            count=$((count + 1)); \
            binary=$candidate; \
        fi; \
    done < /out/candidates; \
    if [ "$count" -ne 1 ]; then \
        printf 'Expected exactly one built executable outside tests/, found %s\n' "$count" >&2; \
        exit 1; \
    fi; \
    install -m 0755 "$binary" /out/compiler; \
    if readelf -l /out/compiler | grep -q INTERP; then \
        ldd /out/compiler > /out/dependencies; \
        if grep -q 'not found' /out/dependencies; then \
            cat /out/dependencies >&2; \
            exit 1; \
        fi; \
        awk '/=> \// { print $3 }' /out/dependencies > /out/libraries; \
        while IFS= read -r library; do \
            cp -L "$library" /out/lib/; \
        done < /out/libraries; \
    fi

# Stage 3: Debian runtime with the compiler and its libraries, without tools.
FROM ${DEBIAN_IMAGE} AS runtime
WORKDIR /app
COPY --from=build /out/compiler /app/compiler
COPY --from=build /out/lib/ /app/lib/
ENV LD_LIBRARY_PATH=/app/lib
USER 65532:65532
ENTRYPOINT ["/app/compiler"]
DOCKERFILE

# Prefer a working container engine, not merely an installed client.
for engine in docker podman; do
    if command -v "$engine" >/dev/null 2>&1 && "$engine" info >> "$log" 2>&1; then runtime=$engine; break; fi
done
if [[ -n $runtime ]]; then
    image_tag="ifj26-check:${tmp_dir##*.}"
    printf 'Překládám pomocí %s (log: %s)...\n' "$runtime" "$log"
    if "$runtime" build -f "$tmp_dir/Dockerfile" -t "$image_tag" "$project" >> "$log" 2>&1; then
        ok 'Projekt úspěšně přeložen (v kontejneru), testuji spuštění'
        if "$runtime" run --rm -i "$image_tag" < "$tmp_dir/example.ifj" > "$tmp_dir/example.ifjcode" 2>> "$log"; then
            ok 'Překladač akceptoval triviální program v jazyce IFJ26'
        else error 'Nebylo možné spustit překlad triviálního programu (viz log)!'; fi
    else error 'Projekt není možné přeložit! (Vyzkoušejte na Merlinovi - pokud to tam půjde, reportujte na iondryas@fit.vut.cz.)'; fi
elif [[ $(hostname) == merlin.fit.vutbr.cz ]]; then
    if ! command -v readelf >/dev/null 2>&1; then
        error 'Je vyžadován program readelf'
    elif (cd "$project" && make) >> "$log" 2>&1; then
        binaries=()
        while IFS= read -r -d '' candidate; do
            if readelf -h "$candidate" 2>/dev/null | grep -Eq 'Type:.*(EXEC|DYN)' \
                && readelf -h "$candidate" 2>/dev/null | grep -Eq 'Entry point address:.*0x[1-9a-fA-F]'; then
                binaries+=("$candidate")
            fi
        done < <(find "$project" -path "$project/tests" -prune -o -type f -perm /111 -print0)
        if ((${#binaries[@]} == 1)); then
            ok "Projekt přeložen, nalezen spustitelný soubor: ${binaries[0]#"$project/"}"
            if (cd "$project" && "${binaries[0]}") < "$tmp_dir/example.ifj" > "$tmp_dir/example.ifjcode" 2>> "$log"; then
                ok 'Překladač akceptoval triviální program v jazyce IFJ26'
            else error 'Nebylo možné spustit překlad triviálního programu (viz log)!'; fi
        else error "Překlad projektu by měl vyprodukovat právě jeden binární spustitelný soubor (mimo adresář tests/), nalezeno: ${#binaries[@]}"; fi
    else error 'Projekt není možné přeložit (chyba make, viz log)!'; fi
else
    warning 'Není možné vyzkoušet přeložitelnost! Nainstalujte Docker (ověřte, že jej váš uživatel má právo používat) či Podman; nebo vyzkoušejte skript na Merlinovi.'
fi

printf "\n==== Výsledek ====\n"

if ((errors)); then
    if ((errors == 1)); then
        printf 'Došlo k 1 chybě!\n' | tee -a "$log"
    else
        printf 'Došlo k %s chybám!\n' "$errors" | tee -a "$log"
    fi
    exit 1
fi
printf 'Všechny kontroly proběhly úspěšně!\n' | tee -a "$log"
