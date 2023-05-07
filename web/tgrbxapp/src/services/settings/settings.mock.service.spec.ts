import { TestBed } from '@angular/core/testing';

import { SettingsMockService } from './settings.mock.service';

describe('SettingsMockService', () => {
  let service: SettingsMockService;

  beforeEach(() => {
    TestBed.configureTestingModule({});
    service = TestBed.inject(SettingsMockService);
  });

  it('should be created', () => {
    expect(service).toBeTruthy();
  });
});
