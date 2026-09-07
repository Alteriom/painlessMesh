# AlteriomPainlessMesh Documentation

> The map of every piece of documentation in this repository, and where it is published.

## Start here

| Document | What it is for |
|----------|----------------|
| [README.md](../README.md) | Overview, installation, quick start, package types, quick API reference |
| [USER_GUIDE.md](../USER_GUIDE.md) | The complete guide: concepts, API reference, Alteriom packages, bridge, advanced features, troubleshooting |
| [BRIDGE_TO_INTERNET.md](../BRIDGE_TO_INTERNET.md) | Connecting a mesh to a router: bridge, channel following, failover, shared gateway, message queue |
| [CHANGELOG.md](../CHANGELOG.md) | What changed in each version, and what to know before upgrading |
| [SECURITY.md](../SECURITY.md) | Threat model, what the library does and does not protect, how to report |
| [RELEASE_GUIDE.md](../RELEASE_GUIDE.md) | How a version is validated, tagged and published |
| [CONTRIBUTING.md](../CONTRIBUTING.md) | Branches, pull requests, tests |

## The documentation site

The site at [alteriom.github.io/painlessMesh](https://alteriom.github.io/painlessMesh/)
is built by `.github/workflows/docs.yml` from two sources:

- the Markdown pages under [`docsify-site/`](../docsify-site/), rendered by
  docsify in the browser (the page shows "Loading…" to a client without
  JavaScript);
- the Doxygen API reference generated from `src/` with
  [`doxygen/Doxyfile`](../doxygen/Doxyfile), published under `api-reference/`.

| Section | Source page | Online |
|---------|-------------|--------|
| Installation | [docsify-site/getting-started/installation.md](../docsify-site/getting-started/installation.md) | https://alteriom.github.io/painlessMesh/#/getting-started/installation |
| Quick start | [docsify-site/getting-started/quickstart.md](../docsify-site/getting-started/quickstart.md) | https://alteriom.github.io/painlessMesh/#/getting-started/quickstart |
| First mesh | [docsify-site/getting-started/first-mesh.md](../docsify-site/getting-started/first-mesh.md) | https://alteriom.github.io/painlessMesh/#/getting-started/first-mesh |
| Core API | [docsify-site/api/core-api.md](../docsify-site/api/core-api.md) | https://alteriom.github.io/painlessMesh/#/api/core-api |
| Callbacks | [docsify-site/api/callbacks.md](../docsify-site/api/callbacks.md) | https://alteriom.github.io/painlessMesh/#/api/callbacks |
| Configuration | [docsify-site/api/configuration.md](../docsify-site/api/configuration.md) | https://alteriom.github.io/painlessMesh/#/api/configuration |
| Doxygen API | [docsify-site/api/doxygen.md](../docsify-site/api/doxygen.md) | https://alteriom.github.io/painlessMesh/#/api/doxygen |
| Alteriom packages | [docsify-site/alteriom/packages.md](../docsify-site/alteriom/packages.md) | https://alteriom.github.io/painlessMesh/#/alteriom/packages |
| Mesh architecture | [docsify-site/architecture/mesh-architecture.md](../docsify-site/architecture/mesh-architecture.md) | https://alteriom.github.io/painlessMesh/#/architecture/mesh-architecture |
| Plugin system | [docsify-site/architecture/plugin-system.md](../docsify-site/architecture/plugin-system.md) | https://alteriom.github.io/painlessMesh/#/architecture/plugin-system |
| Basic examples | [docsify-site/tutorials/basic-examples.md](../docsify-site/tutorials/basic-examples.md) | https://alteriom.github.io/painlessMesh/#/tutorials/basic-examples |
| FAQ | [docsify-site/troubleshooting/faq.md](../docsify-site/troubleshooting/faq.md) | https://alteriom.github.io/painlessMesh/#/troubleshooting/faq |
| Common issues | [docsify-site/troubleshooting/common-issues.md](../docsify-site/troubleshooting/common-issues.md) | https://alteriom.github.io/painlessMesh/#/troubleshooting/common-issues |

## Examples

Eighteen example directories, twenty-one sketches, all compiled for esp32 and
esp8266 on every pull request. Each is listed in `library.json` so the
PlatformIO registry shows it.

| Example | Shows |
|---------|-------|
| [startHere](../examples/startHere/) | The smallest complete mesh sketch |
| [basic](../examples/basic/) | Callbacks, tasks, node list |
| [namedMesh](../examples/namedMesh/) | Addressing nodes by name |
| [alteriom](../examples/alteriom/) | SensorPackage, CommandPackage, StatusPackage; `mppt_example/` shows a custom package |
| [reliableSensorLogging](../examples/reliableSensorLogging/), [commandControl](../examples/commandControl/) | Delivery confirmation (2.0) |
| [priority](../examples/priority/) | Message priorities, with and without a queue |
| [tcpRetryConfig](../examples/tcpRetryConfig/) | Tuning TCP connection retries |
| [bridge](../examples/bridge/) | A bridge to a router with automatic channel detection |
| [bridge_failover](../examples/bridge_failover/) | Automatic bridge election and failover |
| [sharedGateway](../examples/sharedGateway/) | Every node on the router, with automatic relay |
| [sendToInternet](../examples/sendToInternet/) | HTTP requests through the gateway, plus a mock server and a PC node |
| [mqttBridge](../examples/mqttBridge/) | Mesh to MQTT broker |
| [webServer](../examples/webServer/) | A web page that broadcasts into the mesh |
| [otaSender](../examples/otaSender/), [otaReceiver](../examples/otaReceiver/) | Firmware distribution over the mesh |
| [logServer](../examples/logServer/), [logClient](../examples/logClient/) | Central logging |

## Where the library is published

| Channel | State |
|---------|-------|
| [GitHub Releases](https://github.com/Alteriom/painlessMesh/releases) | Every version, with the Arduino ZIP |
| [PlatformIO Registry](https://registry.platformio.org/libraries/alteriom/AlteriomPainlessMesh) | `alteriom/AlteriomPainlessMesh` |
| [npm](https://www.npmjs.com/package/@alteriom/painlessmesh) | `@alteriom/painlessmesh` |
| Arduino Library Manager | `Alteriom PainlessMesh`; the index follows a GitHub release within a day |

## Related repositories

- [alteriom-esp32-farm](https://github.com/Alteriom/alteriom-esp32-farm) — the hardware-in-the-loop rig that validates releases
- [painlessMesh-simulator](https://github.com/Alteriom/painlessMesh-simulator) — multi-node simulation of the examples

## Contributing to the documentation

Fix what you find: a wrong claim, a dead link, an example that does not match
the API. Every code snippet in the guides must compile against the current
headers; when an API changes, the guide changes in the same pull request.
See [CONTRIBUTING.md](../CONTRIBUTING.md).

*Documentation version: 2.0.0*
