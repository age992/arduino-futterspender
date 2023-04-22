import { TestBed } from '@angular/core/testing';

import { StatusMockService } from './status.mock.service';

describe('StatusMockService', () => {
  let service: StatusMockService;

  beforeEach(() => {
    TestBed.configureTestingModule({});
    service = TestBed.inject(StatusMockService);
  });

  it('should be created', () => {
    expect(service).toBeTruthy();
  });
});
