import ifj26.*

@main
def main(): Unit =
  var stop = false
  var total = 0
  for i <- 1 to 10 do
    if !stop then
      total = total + i
      if total > 10 then
        stop = true
  print(total)
