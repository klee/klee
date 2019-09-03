FROM grafana/grafana

RUN grafana-cli plugins install grafana-simple-json-datasource

COPY grafana.ini /etc/grafana/grafana.ini
COPY dashboard.yml /etc/grafana/provisioning/dashboards/dashboard.yml
COPY datasource.yml /etc/grafana/provisioning/datasources/datasource.yml
COPY klee_dashboard.json /var/lib/grafana/dashboards/klee.json

ENTRYPOINT ["/run.sh"]
