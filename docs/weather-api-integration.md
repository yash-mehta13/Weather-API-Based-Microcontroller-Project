# Weather API Integration

Provider: OpenWeatherMap, Current Weather endpoint `https://api.openweathermap.org/data/2.5/weather` (the source used the `http://` form). Official docs: https://openweathermap.org/current.

> This was written from the source and general knowledge; the sandbox could not reach openweathermap.org, so the endpoint, limits and field names were **not re-verified against the live documentation**, and no live request was made. Check the docs and your plan's limits.

## Request
Query parameters: `q` (city, from `WEATHER_CITY`, URL-encoded), `appid` (from `WEATHER_API_KEY`), `units=metric`. The key is a secret even though it travels as a query parameter: it is never logged, and no example URL with a key is published.

## Response fields
| Field | Use | If missing/invalid |
|---|---|---|
| `main.temp` (°C) | display | request fails (WX_FIELDS) |
| `main.humidity` (%) | display | request fails |
| `rain.1h` / `rain.3h` (mm) | rain state | object present but no number → rain **unknown** → pump OFF |
| `rain` absent | valid response | no rain reported |
| `weather[0].id` 200-531 | rain state | ignored if absent |

Temperature must be -40..80, humidity 0..100.

## Status handling
200 parse; 401 bad key; 404 city not found; 429 rate limit; other codes or transport errors (<=0) are failures. Failures never mean "no rain". Retry after 60 s; poll interval 10 min (well below typical free-tier limits; confirm yours).

## HTTPS
`WEATHER_USE_HTTPS 1` uses `WiFiClientSecure` with `setCACert(WEATHER_ROOT_CA_PEM)`; certificate checking stays on and `setInsecure()` is never used. You must supply the root CA that signs the server certificate chain, e.g. inspect it with `openssl s_client -showcerts -connect api.openweathermap.org:443` and paste the PEM of the root into `secrets.h`. Root certificates expire and change; plan to refresh them. The default is HTTP (original behaviour), which exposes the API key on the network. The HTTPS path is compile-untested on a real core.
