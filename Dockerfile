ARG PYTHON_VERSION=3.10
FROM python:${PYTHON_VERSION}-slim

COPY templates /app/templates
COPY gen.py parser.py /app/
COPY requirements.txt /app/
RUN pip install -r requirements.txt

WORKDIR /app

ENTRYPOINT ["python", "parser.py"]
CMD ["generated"]
