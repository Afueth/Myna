# Sources

Every concept in `concepts.csv` has a checklist (`requirements` and
`done_when`). The items are short and written in Myna's own words, not copied
from the sources. They are built on the references below, and
[`source_coverage.csv`](source_coverage.csv) records which concept covers
each part of them. `scripts/check_catalogue.py` checks that file on every
push.

## Coverage

| Reference | Items | Covered | Left out |
|---|---|---|---|
| [W3C ARIA Authoring Practices Guide](https://www.w3.org/WAI/ARIA/apg/patterns/) | 30 patterns | All 30 | None |
| [WCAG 2.2](https://www.w3.org/TR/WCAG22/), levels A and AA | 56 success criteria | 55 | 4.1.1 Parsing, which WCAG 2.2 made obsolete |
| [OWASP Cheat Sheet Series](https://cheatsheetseries.owasp.org/) | 137 cheat sheets | 88 | 49, see below |
| [OWASP MASVS](https://mas.owasp.org/MASVS/) v2 | 24 controls | All 24 | None |
| [Core Web Vitals](https://web.dev/articles/vitals) (web.dev) | 3 metrics, 3 supporting metrics and the assessment rule | All 7 | None |
| [NIST SP 800-63B-4](https://pages.nist.gov/800-63-4/sp800-63b.html) | 14 rules for passwords, sessions and authenticators | 13 | Biometric attempt limits, which the platform enforces |
| API guidelines | 106 topics and proposals | 69 | 37, see below |
| [component.gallery](https://component.gallery/components/) | 59 components | 56 | File, Quote and Separator, plain elements with no behaviour to check |
| [GOV.UK Design System](https://design-system.service.gov.uk/patterns/) | 30 patterns | 27 | Bank details, equality information and National Insurance numbers, which are UK-specific |
| [U.S. Web Design System](https://designsystem.digital.gov/components/overview/) | 47 components | 44 | Banner and Identifier, which are US government identity, and Summary box, a styled box with no behaviour to check |

**W3C ARIA Authoring Practices** describes how UI components such as menus,
tabs, dialogs, sliders and tree views should behave with a keyboard and with
screen readers. Each pattern maps to a Myna concept.

**WCAG 2.2** is the accessibility standard most laws refer to. Every level A
and AA success criterion is covered by at least one concept, such as
`page-structure`, `accessible-forms`, `color-contrast` or
`media-accessibility`. Level AAA, the enhanced level few sites aim for, is
not tracked.

**OWASP Cheat Sheets** cover application security. The 88 sheets about
building web, mobile, API and AI-powered apps are covered.

**OWASP MASVS** is the verification standard for mobile app security. Its
controls map to `mobile-app-security`, `mobile-app-hardening`,
`data-encryption` and `user-data-privacy`.

**Core Web Vitals** are Google's measures of page experience: Largest
Contentful Paint at most 2.5 s, Interaction to Next Paint at most 200 ms and
Cumulative Layout Shift at most 0.1, at the 75th percentile. They map to
`web-performance`, with details in the image and loading concepts.

**NIST SP 800-63B** is the US standard for authentication. Its rules shape the
password, session, two-factor and passkey checklists.

**API guidelines** come from four sources:
[RFC 9457](https://www.rfc-editor.org/rfc/rfc9457) (problem details for
errors), the [Microsoft](https://github.com/microsoft/api-guidelines) and
[Zalando](https://opensource.zalando.com/restful-api-guidelines/) REST API
guidelines, and Google's [API Improvement Proposals](https://google.aip.dev/general).
They contain hundreds of individual rules, many of them overlapping or about
naming style. Myna covers every **topic** they address (errors, filtering,
pagination, versioning, retries, long-running operations, ETags, batches and
data formats), one row per Zalando chapter, Microsoft section and Google AIP.
Left out are event-streaming chapters, Azure- and Google-specific proposals,
proposals specific to protocol buffers and gRPC, and the guides' own
process documents.

**Design systems** supply the names of everyday components and the patterns
around them. [component.gallery](https://component.gallery/components/)
records what each component is called across dozens of design systems, which
feeds Myna's synonyms. The [GOV.UK Design System](https://design-system.service.gov.uk/patterns/)
and the [U.S. Web Design System](https://designsystem.digital.gov/components/overview/)
are openly licensed and backed by user research. Their guidance shapes the
checklists for forms (names, addresses, phone numbers, passwords, dates) and
for pages such as check your answers, confirmation and error pages. IBM's
[Carbon](https://carbondesignsystem.com/patterns/overview/) patterns and
Google's [Material](https://developer.android.com/develop/ui/compose/components)
components were checked for anything the three lists miss, such as bottom
sheets, floating action buttons and disabled states.

### OWASP cheat sheets left out, and why

These don't describe features someone asks Myna to build, so including them
would add noise to search results without helping anyone:

- **Specialised domains** (4): AI-powered advertising, AML sanctions for AI
  payments, automotive, drones; plus browser extensions as a specialised app type.
- **Framework or language specific** (18): for example Django, Laravel, Rails,
  .NET, Java, PHP, Node.js and Next.js. Myna's checklists are
  framework-neutral; framework awareness is planned for version 0.5.
- **Infrastructure and architecture** (13): for example Kubernetes,
  infrastructure as code, microservices, zero trust and cloud architecture.
- **Team process and reference material** (13): for example threat modeling,
  code review, vulnerability disclosure and security terminology.

The full list, with the reason for each, is in `source_coverage.csv`.

## Other areas

Checklists for performance, backend and data, releases, messaging and
payments follow widely used practice, as documented by MDN Web Docs
(<https://developer.mozilla.org/>), web.dev (<https://web.dev/>) and the
documentation of the tools named in the explanations.

## Accuracy

The checklists are a starting point, not legal or security advice. Specific
numbers (such as a 4.5:1 contrast ratio or a one-hour reset link) come from
the sources above or common practice; adjust them to your own requirements.
All items have been through an independent review against the sources.
Found something wrong or missing? Please open an issue or a pull request.

## License

© 2026 Afueth. The data in this folder is licensed under
[CC BY 4.0](LICENSE): reuse it freely, including commercially, but credit
"Myna (https://github.com/Afueth/Myna) by Afueth". The references above
belong to their owners and are not covered by this license.
