const absoluteFormatter = new Intl.DateTimeFormat("zh-CN", {
  year: "numeric",
  month: "2-digit",
  day: "2-digit",
  hour: "2-digit",
  minute: "2-digit",
  second: "2-digit",
  hour12: false
});

const relativeFormatter = new Intl.RelativeTimeFormat("zh-CN", {
  numeric: "auto"
});

function parseServerDate(input: string): Date | null {
  if (!input) {
    return null;
  }

  const normalized = input.includes("T") ? input : input.replace(" ", "T");
  const hasTimezone = /[zZ]|[+-]\d{2}:\d{2}$/.test(normalized);
  const utcCandidate = hasTimezone ? normalized : `${normalized}Z`;

  const date = new Date(utcCandidate);
  if (!Number.isNaN(date.getTime())) {
    return date;
  }

  const fallback = new Date(input);
  if (!Number.isNaN(fallback.getTime())) {
    return fallback;
  }

  return null;
}

export function formatAbsoluteDateTime(input: string): string {
  const date = parseServerDate(input);
  if (!date) {
    return input;
  }
  return absoluteFormatter.format(date);
}

export function formatRelativeDateTime(input: string): string {
  const date = parseServerDate(input);
  if (!date) {
    return input;
  }

  const deltaMs = date.getTime() - Date.now();
  const deltaMinutes = Math.round(deltaMs / 60000);
  const absMinutes = Math.abs(deltaMinutes);

  if (absMinutes < 1) {
    return "刚刚";
  }
  if (absMinutes < 60) {
    return relativeFormatter.format(deltaMinutes, "minute");
  }

  const deltaHours = Math.round(deltaMinutes / 60);
  const absHours = Math.abs(deltaHours);
  if (absHours < 24) {
    return relativeFormatter.format(deltaHours, "hour");
  }

  const deltaDays = Math.round(deltaHours / 24);
  if (Math.abs(deltaDays) < 14) {
    return relativeFormatter.format(deltaDays, "day");
  }

  return absoluteFormatter.format(date);
}
