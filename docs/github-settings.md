# GitHub Repository Settings

Some community controls cannot be enabled or created by committed repository files. Apply these settings in the GitHub **Settings** page for this repository.

## Enable Discussions

1. Open **Settings → General → Features**.
2. Enable **Discussions** and save.
3. Create categories for **Q&A**, **Ideas**, **Show and tell**, and **Announcements**.
4. Pin a welcome post that explains the split:
   - **Issues:** confirmed bugs, reproducible failures and concrete work items.
   - **Discussions:** usage questions, design exploration, RFC proposals and general ideas.
5. Keep vulnerability reports private through the Security tab; do not request exploit details in public Discussions.

## Synchronize labels

The repository includes `.github/labels.json` and `scripts/sync_labels.py`. Run the **Sync repository labels** workflow from the Actions tab, or execute the script locally with a token that has repository Issues write access:

```sh
export GH_REPO=fanxcz/nexus-lang
export GH_TOKEN=<token-with-issues-write>
python3 scripts/sync_labels.py
```

The script creates missing labels and updates descriptions/colors for existing labels. It does not delete unrelated labels.

## Default branch and protections

Set the default branch to `main`. Protect `main` with pull requests, require the stable quality/build checks that are enabled for the repository, and disallow force pushes and branch deletion. Require review where the maintainer workflow permits it. Do not make optional platform-specific checks required until those jobs are consistently available.

## Repository metadata

Set the project description and topics (`compiler`, `programming-language`, `cpp`, `llvm`, `native-codegen`). Add the project website/docs URL if maintained. Review the Security tab and enable private vulnerability reporting when available.
