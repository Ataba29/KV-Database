# Contributing to ByteForge

Thanks for your interest in contributing. This project is maintained by a small team, so contributions go through forks and pull requests — there's no direct push access for non-collaborators.

## Before you start

- Check open [Issues](../../issues) and [PRs](../../pulls) to avoid duplicate work.
- For anything non-trivial (new feature, architecture change), open an issue first to discuss the approach before writing code.
- Small fixes (typos, docs, obvious bugs) can go straight to a PR.

## Workflow

1. **Fork** the repository (you won't be able to push branches directly unless you're a collaborator).
2. **Branch** off `main` using this naming scheme (Git flow style):
   - `feature/<short-description>`
   - `fix/<short-description>`
   - `chore/<short-description>`
   - `docs/<short-description>`
3. **Build and test locally** before opening a PR (see below).
4. **Commit** with clear, descriptive messages. Keep commits scoped to one logical change.
5. **Open a pull request** against `main` using the PR template. Link the related issue if there is one.
6. Respond to review feedback — PRs need at least one approval and a passing CI run before merge.

## Building the project

ByteForge uses CMake + Ninja and targets C++23.

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

Run the test suite (GoogleTest, fetched via CMake FetchContent):

```bash
ctest --test-dir build --output-on-failure
```

The project builds cross-platform (Linux via epoll, Windows via IOCP) behind a shared `IEventLoop` interface — if your change touches the event loop, please test on both platforms if you can, or note in the PR which platform you tested.

## Code style

- Header guards: `#ifndef` / `#define` / `#endif` (no `#pragma once`).
- Split declarations and definitions into `.h` / `.cpp` files.
- Document public APIs with short Doxygen-style `/** */` comments — keep them concise, not verbose.
- Match the formatting of surrounding code (brace style, naming conventions) rather than introducing a new style in a file.

## Pull request expectations

- One feature/fix per PR — keep diffs reviewable.
- Include or update tests for behavioral changes.
- Update documentation (README, code comments) if the change affects usage or public interfaces.
- CI must pass (build + tests) before merge.
- Don't fabricate benchmark numbers in PR descriptions — if you're citing performance impact, include how you measured it.

## Reporting bugs / requesting features

Use the issue templates — they'll prompt for the info needed to reproduce a bug or evaluate a feature request. Issues from non-collaborators are welcome; just keep them scoped and specific.

## Code of conduct

Be respectful and constructive in issues, PRs, and reviews. Disagreements about technical approach are fine and expected — keep them focused on the code, not the person.

## Questions

Open a [Discussion](../../discussions) or a low-priority issue tagged `question`.
