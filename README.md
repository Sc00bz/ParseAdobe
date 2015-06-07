# ParseAdobe
Parses the cred file from the Adobe dump

## Usage

```
./parseadobe [in-file [out-file [fields]]
```

Default fields is 000100 (output password in hex):<br>
100000 output id<br>
010000 output name<br>
001000 output email<br>
000100 output password in hex<br>
000200 output password in base64<br>
000010 output hint<br>
000001 all fields required

## Examples
Output passwords in hex
```
./parseadobe < cred > cred-out
./parseadobe cred cred-out
```

Output email addresses only
```
./parseadobe cred cred-out 001000
```

Output email addresses and base64 passwords
```
./parseadobe cred cred-out 001200
```

Output hex passwords and hints
```
./parseadobe cred cred-out 000110
```