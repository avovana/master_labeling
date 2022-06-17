# master_labeling
My hobby project made from scratch.
The task was to automate scan process at a product factory.
Decided to implement **master <--> slaves** architecture. Used TCP to communicate. Created special protocol over TCP.

Master part functions:
- Communicate with [slave controllers](https://github.com/avovana/slave_controller)
- Make tasks for them dynamically
- Observe slaves state
- Get scans
- Get final file
- Make final scan templates
<img src="https://github.com/avovana/master_labeling/blob/master/master_slave_work.png" width="700"/>

For monitoring purpose installed and used "PostgreSQL + Grafana" solution:

<img src="https://github.com/avovana/master_labeling/blob/master/grafana.png"/>

To clarify functions behavior while design stage used diagrams:

<img src="https://github.com/avovana/master_labeling/blob/master/otbrakovka.png" width="700"/>
