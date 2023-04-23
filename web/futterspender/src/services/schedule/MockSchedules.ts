import { EScheduleMode } from 'src/app/lib/EScheduleMode';
import { Schedule } from 'src/models/Schedule';

const d1 = new Date('2022-01-01T06:00:00.000Z');
const d2 = new Date('2022-01-01T12:30:00.000Z');
const d3 = new Date('2022-01-01T19:30:00.000Z');

export const schedules: Schedule[] = [
  {
    ID: 0,
    Name: 'Fritzchen Ferien',
    Active: false,
    Selected: false,
    Mode: EScheduleMode.Max,
    OnlyWhenEmpty: true,
    MaxTimes: 3,
  },
  {
    ID: 1,
    Name: 'Fritzchen Normal',
    Active: true,
    Selected: true,
    Mode: EScheduleMode.FixedDaytime,
    OnlyWhenEmpty: false,
    Daytimes: [d1, d2, d3],
  },
];
