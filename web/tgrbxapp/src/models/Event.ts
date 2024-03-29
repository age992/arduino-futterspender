export interface EventData {
  ID: number;
  CreatedOn: number;
  Type: EventType;
  Value: string;
}

export enum EventType {
  Feed,
  MissedFeed,
  ContainerEmpty,
  MotorFaliure,
  WiFiConnectionLost,
  WiFiConnectionReturned,
  SDConnectionLost,
  SDConnectionReturned,
  SkippedFeed,
}
