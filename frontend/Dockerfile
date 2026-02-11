ARG NODE_IMAGE=node:20-alpine
FROM ${NODE_IMAGE} AS builder
WORKDIR /app/frontend
ARG VITE_SITE_URL
ENV VITE_SITE_URL=${VITE_SITE_URL}
COPY frontend/package*.json ./
RUN npm install
COPY frontend/ ./
RUN npm run build

ARG NGINX_IMAGE=nginx:1.27-alpine
FROM ${NGINX_IMAGE}
COPY frontend/nginx.conf /etc/nginx/conf.d/default.conf
COPY --from=builder /app/frontend/dist /usr/share/nginx/html
EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
