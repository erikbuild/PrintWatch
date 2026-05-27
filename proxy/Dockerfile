FROM python:3.12-slim

COPY --from=ghcr.io/astral-sh/uv:latest /uv /usr/local/bin/uv

WORKDIR /app

COPY pyproject.toml uv.lock ./
COPY proxy/ proxy/
RUN uv sync --frozen --no-dev

EXPOSE 8080

CMD ["uv", "run", "--no-dev", "printwatch-proxy", "/config/config.yaml"]
