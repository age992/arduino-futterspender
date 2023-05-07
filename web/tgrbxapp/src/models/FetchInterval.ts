export interface FetchInterval {
  Timer?: NodeJS.Timer;
  Active: boolean;
  IntervalSeconds: number;
}
