<?php
// Order these in order they should be printed
// http://php.net/manual/en/timezones.php
$timezones = [
  'America/New_York',
  'Europe/London',
  'Asia/Manila',
  'Australia/Melbourne',
  'Australia/Adelaide'
];
// This is a stupid break so we know when to start reading
print('!$$%%'."\n");

// Time lib wants seconds since Jan, 1 2000 instead of epoch
// $Y2K = new DateTime('2000-01-01T00:00:00+00:00');
// $Y2KOffset = $Y2K->getTimestamp();
print((time() - 946684800) . "\n");

foreach ($timezones as $tz) {
    $t = new DateTime('now', new DateTimeZone($tz));
    print($t->getOffset() . "\n");
}
