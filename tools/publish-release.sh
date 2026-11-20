#!/usr/bin/env bash
set -euo pipefail
package_dir=${1:-release}
version=${RELEASE_VERSION:?RELEASE_VERSION is required}
release_tag=${RELEASE_TAG:?RELEASE_TAG is required}
if [[ ! $version =~ ^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$ || ( $release_tag != "v$version" && $release_tag != "$version" ) ]]; then
    echo 'Release tag and package version must match.' >&2
    exit 1
fi
assets=(
    "$package_dir/razer-mouse-lite-$version-windows-x64.zip"
    "$package_dir/razer-mouse-lite-$version-windows-x64-setup.exe"
    "$package_dir/razer-mouse-lite-$version-linux-x86_64.deb"
    "$package_dir/razer-mouse-lite-$version-debian13-amd64.deb"
)
for asset in "${assets[@]}"; do
    if [[ ! -s $asset ]]; then echo "Missing release asset: $asset" >&2; exit 1; fi
done
if git show-ref --verify --quiet "refs/tags/$release_tag"; then
    if [[ $(git rev-parse "$release_tag^{commit}") != "$(git rev-parse HEAD)" ]]; then
        echo 'Existing release tag points at a different commit.' >&2
        exit 1
    fi
fi
if existing_assets=$(gh release view "$release_tag" --json assets --jq '.assets[].name'); then
    for asset in "${assets[@]}"; do
        name=${asset##*/}
        if ! grep -Fxq "$name" <<< "$existing_assets"; then gh release upload "$release_tag" "$asset"; fi
    done
else
    gh release create "$release_tag" "${assets[@]}" --target "$(git rev-parse HEAD)" --title "Razer Mouse Lite $release_tag" --generate-notes
fi
