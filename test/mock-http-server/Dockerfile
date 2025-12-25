FROM python:3.11-slim

LABEL maintainer="painlessMesh Contributors"
LABEL description="Mock HTTP Server for painlessMesh Bridge Testing"

WORKDIR /app

# Copy server script
COPY server.py /app/

# Make script executable
RUN chmod +x /app/server.py

# Expose default port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD python3 -c "import urllib.request; urllib.request.urlopen('http://localhost:8080/health')"

# Run server
CMD ["python3", "/app/server.py", "--host", "0.0.0.0", "--port", "8080"]
