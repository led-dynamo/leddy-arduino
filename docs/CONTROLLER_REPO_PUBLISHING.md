# Publishing standalone controller repositories

The canonical ESP32-S3 and STM32F4 sources live in this repository until the
standalone organization repositories are initialized.

## Networked publication

From a trusted shell or CI runner with outbound GitHub access, export a GitHub
token with permission to create repositories in `led-dynamo` and push content:

```sh
export GH_TOKEN='...'
bash scripts/publish-controller-repos.sh all
```

The token is read only from the process environment. The script does not write
it to repository files or Git remote configuration.

The publisher:

1. checks for `led-dynamo/leddy-esp32` and `led-dynamo/leddy-stm32`;
2. creates either missing repository as public and without an auto-generated
   initial commit;
3. materializes the corresponding standalone tree with
   `scripts/extract-controller-repo.sh`;
4. initializes `main`, commits the extracted source, and pushes it;
5. creates and pushes the `dev` integration branch;
6. refuses to overwrite a repository that already contains branches.

Publish one target by passing `esp32` or `stm32` instead of `all`.

## Pre-publication verification

Before publication, the existing CI validates extraction and deterministic
artifacts. A networked operator can also run:

```sh
bash scripts/test-controller-extraction.sh
bash scripts/test-controller-artifacts.sh
bash scripts/package-controller-artifacts.sh /tmp/leddy-controller-artifacts
sha256sum -c /tmp/leddy-controller-artifacts/leddy-esp32.tar.gz.sha256
sha256sum -c /tmp/leddy-controller-artifacts/leddy-stm32.tar.gz.sha256
```

After the two repositories are published and their independent CI passes, add
them to `leddy-monorepo` as optional device-agent repositories. Only after that
migration is verified should the duplicate native source directories be removed
from `leddy-arduino`.

Never commit GitHub, Linear, Cloudflare, or R2 credentials to any repository.
