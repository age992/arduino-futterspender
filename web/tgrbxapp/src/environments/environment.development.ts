import { ScheduleMockService } from 'src/services/schedule/schedule.mock.service';
import { ScheduleService } from 'src/services/schedule/schedule.service';
import { SettingsMockService } from 'src/services/settings/settings.mock.service';
import { SettingsService } from 'src/services/settings/settings.service';
import { StatusMockService } from 'src/services/status/status.mock.service';
import { StatusService } from 'src/services/status/status.service';

export const environment = {
  production: false,
  providers: [
    // { provide: StatusService, useClass: StatusMockService },
    { provide: ScheduleService, useClass: ScheduleMockService },
    { provide: SettingsService, useClass: SettingsMockService },
  ],
  apiUrl: 'http://tigerbox.local/api',
  wsUrl: 'ws://192.168.137.48/ws',
  //apiUrl: 'http://192.168.137.206/api',
};
